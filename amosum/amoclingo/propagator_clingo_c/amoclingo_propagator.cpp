#include "amoclingo_propagator.h"
#include "../../amosum_propagator.h"
#include "fiore/model/solver.h"
#include <map>
#include <algorithm>

AmoclingoPropagator::AmoclingoPropagator(std::string name, fiore::ClingoSolver* solver, AmosumConstraint* constraint, const std::string& reason, const std::string& lazy) noexcept:
    // falsity changes the max/min possible sum (ge, EO); le_amo depends only on the true literals: their negations
    // are not watched, as in the old watch filter of getLiterals
    fiore::PropagatorPBConstraint(name, solver, constraint, constraint->type != "le_amo"),
    constraint(constraint),
    ge(constraint->ge),
    eo(constraint->type == "ge_eo" || constraint->type == "le_eo"),
    constant(0), N(2),
    fullTrueReason(reason == "ijcai"),
    minimizeOnTheFly(reason == "minfly" || reason == "min"),
    lazyActive(lazy != SETTINGS::FALSE_STR),
    lazyThreshold(lazy == SETTINGS::FALSE_STR || lazy == SETTINGS::LAZY_HYBRID),
    lazyPerc(lazy == SETTINGS::TRUE_STR ? 1.0 : (lazyThreshold ? 0.0 : std::stod(lazy))) {

}

fiore::PropagatorPBConstraintImplementation* AmoclingoPropagator::createImplementation() noexcept { return new AmosumPropagator(this); }

void AmoclingoPropagator::initFirstTime(clingo_propagate_init* _init) noexcept{
    // not PropagatorPBConstraint::initFirstTime: it maps only the soft literals of the program and has no groups
    solver->initializer->init(_init, solver);

    auto solverLiteral = [_init](const clingo_literal_t plit){
        clingo_literal_t slit;
        handleError(clingo_propagate_init_solver_literal(_init, std::abs(plit), &slit));
        return plit > 0 ? slit : fiore::not_(slit);
    };

    // 1a. elements -> solver literals: equal solver literals are merged (weights summed) into the first group containing
    //    them; a later group containing the same solver literal keeps it as a moved literal (weight 0 there)
    const size_t nGroups = constraint->groups.size();
    std::vector<std::vector<clingo_literal_t>> members(nGroups);          // members of each group
    std::vector<std::vector<clingo_literal_t>> moved(nGroups);            // moved literals of each group (weight 0 there)
    std::vector<std::unordered_set<clingo_literal_t>> movedSet(nGroups);  // the same, for membership checks
    std::map<clingo_literal_t, size_t> owner;                             // solver literal -> its owner group
    std::map<clingo_literal_t, int64_t> merged;                           // solver literal -> sum of the weights of its elements
    std::map<clingo_literal_t, std::unordered_set<const fiore::Literal*>> elements;   // solver literal -> its elements
    std::vector<bool> decided(nGroups, false);

    for(size_t gi = 0; gi < nGroups; ++gi){
        for(const auto& [plitLiteral, w]: constraint->groups[gi].members){
            const clingo_literal_t plit = plitLiteral->id;
            const clingo_literal_t slit = solverLiteral(plit);
            if(slit == fiore::CONSTANTS::BOTTOM) continue;                   // never true: contributes 0
            if(slit == fiore::CONSTANTS::TOP){ decided[gi] = true; constant += w; continue; }
            auto [it, inserted] = owner.emplace(slit, gi);
            if(inserted) members[gi].push_back(slit);
            else if(it->second != gi && movedSet[gi].insert(slit).second) moved[gi].push_back(slit);
            merged[slit] += w;
            elements[slit].insert(constraint->program->mapLiteralId2Literal->literal(plit));
        }
    }

    // 1b. negative weights: the propagator needs w >= 0 (0 is the least value of a group), so w·s = |w|·¬s − |w| (w < 0):
    //     s stays in its group as a moved literal (s true: the group's true element, worth 0 there; keeps EO groups
    //     exactly-one), |w|·¬s is one more element (merged as the others, in a new group if ¬s is not owned yet) and the
    //     constant takes −|w|: the value of the aggregate is the same in every assignment
    std::vector<std::string> ids;
    std::vector<bool> aux;
    for(const ProgramGroup& g: constraint->groups){ ids.push_back(g.id); aux.push_back(g.aux); }
    std::vector<clingo_literal_t> negatives;
    std::map<clingo_literal_t, size_t> movedFrom;          // literal turned into a moved literal by 1b -> its former owner group
    for(const auto& [slit, w]: merged) if(w < 0) negatives.push_back(slit);
    while(!negatives.empty()){
        const clingo_literal_t s = negatives.back();
        negatives.pop_back();
        // still negative? (¬s, processed before, may have raised it)
        auto itw = merged.find(s);
        if(itw == merged.end() || itw->second >= 0) continue;
        const int64_t w = itw->second;
        const size_t gi = owner.at(s);
        // s: from member to moved literal of its group
        if(movedSet[gi].insert(s).second) moved[gi].push_back(s);
        movedFrom.emplace(s, gi);
        owner.erase(s);
        merged.erase(itw);
        constant -= std::abs(w);
        // |w|·¬s: merged into the owner of ¬s; if ¬s was turned into a moved literal here before, it goes back to being
        // a member of that group (its former owner: at most one true still holds there); otherwise a new group
        const clingo_literal_t ns = fiore::not_(s);
        if(owner.find(ns) == owner.end()){
            if(auto mf = movedFrom.find(ns); mf != movedFrom.end()){
                const size_t gk = mf->second;
                owner.emplace(ns, gk);
                movedSet[gk].erase(ns);                       // leaves moved[gk] (filtered in step 3)
                movedFrom.erase(mf);
            }else{
                owner.emplace(ns, members.size());
                members.push_back({ns});
                moved.emplace_back();
                movedSet.emplace_back();
                decided.push_back(false);
                ids.push_back(ids[gi] + "/not");
                aux.push_back(false);
            }
        }
        // owner and merged always hold the same literals: ns missing from merged (turned moved before) starts from 0
        merged[ns] += std::abs(w);
        for(const fiore::Literal* l: elements[s]) elements[ns].insert(constraint->program->mapLiteralId2Literal->literal(fiore::not_(l->id)));
    }

    // 2. a group with a fact is decided: by the AMO/EO promise its other elements are false
    for(size_t gi = 0; gi < members.size(); ++gi) if(decided[gi]){ members[gi].clear(); moved[gi].clear(); }

    // 3. groups over solver literals, ordered by (weight, literal) as in amosum_initializer.cpp; a group without own
    //    literals contributes 0 (its true element, if any, is counted in another group)
    clingo_atom_t maxAtom = 1;
    for(const auto& [slit, gi]: owner) maxAtom = std::max<clingo_atom_t>(maxAtom, std::abs(slit));
    N = maxAtom + 1;
    weight.reset(new fiore::PerfectNHash<clingo_literal_t, int64_t>(N, 0));
    groupIndex.reset(new fiore::PerfectNHash<clingo_literal_t, size_t>(N, NO_GROUP));
    for(size_t gi = 0; gi < members.size(); ++gi){
        // the members turned into moved literals of this group by 1b leave it: one pass instead of one erase each
        const std::unordered_set<clingo_literal_t>& ms = movedSet[gi];
        if(!ms.empty()) members[gi].erase(std::remove_if(members[gi].begin(), members[gi].end(), [&ms](const clingo_literal_t l){ return ms.count(l) > 0; }), members[gi].end());
        // the moved literals that went back to being members (1b) leave moved
        if(moved[gi].size() != ms.size()) moved[gi].erase(std::remove_if(moved[gi].begin(), moved[gi].end(), [&ms](const clingo_literal_t l){ return ms.count(l) == 0; }), moved[gi].end());
        if(members[gi].empty()) continue;
        
        AmosumSolvingGroup g{ids[gi], std::move(members[gi]), std::move(moved[gi]), aux[gi]};
        for(const clingo_literal_t slit: g.literals) weight->set(slit, merged[slit]);
        std::sort(g.literals.begin(), g.literals.end(), [this](const clingo_literal_t a, const clingo_literal_t b){
            return weight->at(a) != weight->at(b) ? weight->at(a) < weight->at(b) : a < b;
        });
        const size_t index = solvingGroups.size();
        for(const clingo_literal_t slit: g.literals) groupIndex->set(slit, index);
        for(const clingo_literal_t slit: g.movedLiterals) movedInto[slit].push_back(index);
        solvingGroups.push_back(std::move(g));
    }
    hasMovedLiterals = !movedInto.empty();

    // 4. watched literals (initImplementation watches each one, and its negation if watchOppositePolarityLiterals):
    //    one per atom when both polarities are watched, one per literal otherwise (s and -s may both be members);
    //    a moved literal is watched even when its owner group was dropped
    ownedSolvingLiterals.clear();
    std::unordered_set<clingo_literal_t> watchedKeys;
    for(const AmosumSolvingGroup& g: solvingGroups){
        for(const std::vector<clingo_literal_t>* lits: {&g.literals, &g.movedLiterals}){
            for(const clingo_literal_t slit: *lits){
                const clingo_literal_t key = watchOppositePolarityLiterals ? std::abs(slit) : slit;
                if(!watchedKeys.insert(key).second) continue;
                memberSolvingLiterals.emplace_back(new fiore::SolvingLiteral(slit, elements[slit]));
                ownedSolvingLiterals.push_back(memberSolvingLiterals.back().get());
            }
        }
    }
    // the implementations exist whenever watches are added (initImplementation adds them after this function, even
    // after the returns below)
    for(size_t ni = 0; ni < solver->initializer->nt; ++ni) implementations.emplace_back(createImplementation());

    // 5. the __group__ atoms only carry the description of the constraint: false, as in the old simplifyAtLevelZero
    for(const clingo_literal_t plit: constraint->groupsAtoms){
        const clingo_literal_t lit = fiore::not_(solverLiteral(plit));
        bool result;
        handleError(clingo_propagate_init_add_clause(_init, &lit, 1, &result));
        if(!result) return;
    }

    // 6. the bound with every literal still undefined: violated by the facts alone -> no answer set
    int64_t initialMps = constant;
    if(ge || eo){
        for(const AmosumSolvingGroup& g: solvingGroups)
            initialMps += ge ? weight->at(g.literals.back()) : (g.movedLiterals.empty() ? weight->at(g.literals.front()) : 0);
    }
    if(ge ? initialMps < constraint->bound : initialMps > constraint->bound){
        const clingo_literal_t bottom = fiore::CONSTANTS::BOTTOM;
        bool result;
        handleError(clingo_propagate_init_add_clause(_init, &bottom, 1, &result));
        return;
    }

    
}