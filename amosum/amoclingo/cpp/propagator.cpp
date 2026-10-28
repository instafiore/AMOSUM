#include "amosum/amoclingo/cpp/propagator.h"
#include "fiore/model/solver.h"
#include <unordered_set>

AmoclingoPropagator::AmoclingoPropagator(std::string name, fiore::ClingoSolver* solver, AmosumConstraint* constraint, const std::string& reason, const std::string& lazy, const clingo_clause_type_t clauseType) noexcept:
    // falsity changes the max/min possible sum (ge, EO); le_amo depends only on the true literals: their negations
    // are not watched, as in the old watch filter of getLiterals
    fiore::PropagatorPBConstraint(name, solver, constraint, constraint->type != "le_amo", clauseType),
    constraint(constraint),
    ge(constraint->ge),
    eo(constraint->type == "ge_eo" || constraint->type == "le_eo"),
    options(reason, lazy){}

fiore::PropagatorPBConstraintImplementation* AmoclingoPropagator::createImplementation() noexcept {
    return new AmosumPropagator(this, solvingGroups, options, ge, eo, solver->initializer->nt);
}

void AmoclingoPropagator::initFirstTime(clingo_propagate_init* _init) noexcept{
    // not PropagatorPBConstraint::initFirstTime: it maps only the soft literals of the program and has no groups
    solver->initializer->init(_init, solver);

    auto solverLiteral = [_init](const clingo_literal_t plit){
        clingo_literal_t slit;
        handleError(clingo_propagate_init_solver_literal(_init, std::abs(plit), &slit));
        return plit > 0 ? slit : fiore::not_(slit);
    };

    // 1-3. the groups over solver literals (core)
    solvingGroups = buildSolvingGroups(*constraint, solverLiteral);

    // 4. watched literals (initImplementation watches each one, and its negation if watchOppositePolarityLiterals):
    //    one per atom when both polarities are watched, one per literal otherwise (s and -s may both be members);
    //    a moved literal is watched even when its owner group was dropped
    ownedSolvingLiterals.clear();
    std::unordered_set<clingo_literal_t> watchedKeys;
    for(const AmosumSolvingGroup& g: solvingGroups.groups){
        for(const std::vector<clingo_literal_t>* lits: {&g.literals, &g.movedLiterals}){
            for(const clingo_literal_t slit: *lits){
                const clingo_literal_t key = watchOppositePolarityLiterals ? std::abs(slit) : slit;
                if(!watchedKeys.insert(key).second) continue;
                memberSolvingLiterals.emplace_back(new fiore::SolvingLiteral(slit, solvingGroups.elements[slit]));
                ownedSolvingLiterals.push_back(memberSolvingLiterals.back().get());
            }
        }
    }
    // the implementations exist whenever watches are added (initImplementation adds them after this function, even
    // after the returns below)
    for(size_t ni = 0; ni < solver->initializer->nt; ++ni) implementations.emplace_back(createImplementation());

    // 5. the __group__ atoms only carry the description of the constraint: false, as in the old simplifyAtLevelZero
    for(const clingo_literal_t plit: constraint->groupsAtoms){
        if(addClause(_init, {fiore::not_(solverLiteral(plit))})) return;
    }
}
