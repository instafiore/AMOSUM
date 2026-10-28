#include "amosum_program.h"
#include <algorithm>
#include <map>
#include "fiore/model/program.h"
#include "fiore/model/assigment.h"

// Anonymus namespace
namespace {
    struct GroupElement{
        const fiore::Literal* literal;
        int64_t weight;
        std::string groupId;
        std::string tuple;          // second argument of __element__(ID, T, G); "" for the aux element
        bool aux;                   // the "-" element
    };

    struct AggregateData{
        std::vector<GroupElement> elements;
        std::vector<clingo_literal_t> groupsAtoms;
        int64_t bound = 0;
        bool ge = true;
        std::string type;
    };
}

fiore::Program* AmosumProgram::AmosumGrounder::createProgram() const{
    return new AmosumProgram();
}



namespace {

    struct NormalizeResult{
        std::vector<ProgramGroup> groups;
        fiore::UMNWeightFunction* w;
    };

    // program level normalization of an aggregate: a tuple in several groups counts once, equal weights in a group are one atom
    NormalizeResult normalize(AmosumProgram* p, const std::string& aggregateId, const bool eo, const std::vector<GroupElement>& elements){
        std::map<std::string, ProgramGroup> byId;                      // deterministic order
        std::map<std::string, std::vector<const GroupElement*>> byTuple;
        
        for(const GroupElement& e: elements){
            ProgramGroup& g = byId[e.groupId];
            g.id = e.groupId;
            g.aux = g.aux || e.aux;
            g.members.emplace_back(e.literal, e.weight);
            if(!e.tuple.empty()) byTuple[e.tuple].push_back(&e);
        }

        // 2. a tuple in several groups: x = OR of its element atoms, weight w in a group of its own; the element atoms stay in
        //    their groups with weight 0 (the tuple is counted once, by x); EO: ¬x in x's group too, so that it is exactly-one
        //    (le_eo: its minimum is 0 while x is not true; ge_eo: the lazy threshold sees that the group can change by w)
        std::vector<ProgramGroup> extra;
        for(const auto& [tuple, es]: byTuple){
            if(es.size() < 2) continue;
            std::vector<const fiore::Literal*> lits;
            for(const GroupElement* e: es){
                lits.push_back(e->literal);
                ProgramGroup& g = byId[e->groupId];
                std::find_if(g.members.begin(), g.members.end(), [e](const auto& m){ return m.first == e->literal; })->second = 0;
            }
            const fiore::Literal* x = p->addDisjunction(lits, "__tuple__(" + aggregateId + "," + tuple + ")");
            ProgramGroup g{"__tuple__" + tuple, {{x, es.front()->weight}}};
            if(eo) g.members.emplace_back(p->mapLiteralId2Literal->literal(fiore::not_(x->id)), 0);
            extra.push_back(std::move(g));
            p->maxLiteralId = std::max(p->maxLiteralId, static_cast<uint32_t>(std::abs(x->id)));
        }

        // 3. equal weights in a group: y = OR of those members, with that weight (at most one true: w·[m1] + w·[m2] = w·[y])
        for(auto& [id, g]: byId){
            std::map<int64_t, std::vector<const fiore::Literal*>> byWeight;
            for(const auto& [l, w]: g.members) byWeight[w].push_back(l);
            std::vector<std::pair<const fiore::Literal*, int64_t>> members;
            for(const auto& [w, ls]: byWeight){
                if(ls.size() == 1){ members.emplace_back(ls.front(), w); continue; }
                members.emplace_back(p->addDisjunction(ls, "__same__(" + aggregateId + "," + id + "," + std::to_string(w) + ")"), w);
            }
            g.members = std::move(members);
        }

        std::vector<ProgramGroup> groups;
        for(auto& [id, g]: byId) groups.push_back(std::move(g));
        for(ProgramGroup& g: extra) groups.push_back(std::move(g));
        // fiore::UMNWeightFunction* w = new fiore::UMNWeightFunction(p->maxLiteralId+1);
        // for(ProgramGroup& g: groups){
        //     for(const auto&[literal, weight]: g.members){
        //             w->set(literal->id, weight);
        //     }
        // }
        fiore::UMNWeightFunction* w = new fiore::UMNWeightFunction(1);
        return {groups, w};
    }
}

fiore::Program* AmosumProgram::AmosumGrounder::ground(const std::vector<std::string>& ASPInputs){
    AmosumProgram* p = static_cast<AmosumProgram*>(fiore::Program::ProgramGrounder::ground(ASPInputs));
    p->amomaximizeConstraint = nullptr;
    // aggregate id -> data, ordered by id so that the constraints are created in a deterministic order
    std::map<std::string, AggregateData> aggregates;

    for(const fiore::Literal* plit: *p->restLiterals.get()){
        LOG(fiore::LogLevel::DEBUG, "Literal: ", plit);

        clingo_symbol_t symbol;
        handle_error(clingo_parse_term(plit->name.c_str(), nullptr, nullptr, 0, &symbol));
        if(clingo_symbol_type(symbol) != clingo_symbol_type_function) continue;

        char const* name;
        handle_error(clingo_symbol_name(symbol, &name));
        clingo_symbol_t const* args;
        size_t n;
        handle_error(clingo_symbol_arguments(symbol, &args, &n));
        const std::string predicate(name);

        if(predicate == SETTINGS::PREDICATE_GROUP && n == 5){
            // __group__(L, "+"|"-", W, G, ID)
            AggregateData& a = aggregates[from_symbol_to_string(args[4])];
            const fiore::Literal* lit = p->literalByName(from_symbol_to_string(args[0]));
            if(lit == nullptr) continue;
            const bool plus = from_symbol_to_string(args[1]) == "\"+\"";
            if(!plus) lit = p->mapLiteralId2Literal->literal(not_(lit->id));
            
            int w;
            handle_error(clingo_symbol_number(args[2], &w));
            // the tuple of the element atom __element__(ID, T, G); "" for the aux element (__aux__): no ground term prints
            // as the empty string (even the empty tuple is "()"), so "" can't be a real tuple
            std::string tuple = "";
            char const* elementName;
            handle_error(clingo_symbol_name(args[0], &elementName));
            clingo_symbol_t const* elementArgs;
            size_t elementN;
            handle_error(clingo_symbol_arguments(args[0], &elementArgs, &elementN));
            if(std::string(elementName) == "__element__" && elementN == 3) tuple = from_symbol_to_string(elementArgs[1]);
            a.elements.push_back({lit, w, from_symbol_to_string(args[3]), tuple, !plus});   // aux = !plus
            a.groupsAtoms.push_back(plit->id);
        }
        else if((predicate == SETTINGS::PREDICATE_LB || predicate == SETTINGS::PREDICATE_UB) && n == 2){
            // __lb__(B, ID) / __ub__(B, ID)
            AggregateData& a = aggregates[from_symbol_to_string(args[1])];
            int b;
            handle_error(clingo_symbol_number(args[0], &b));
            a.bound = b;
            a.ge = predicate == SETTINGS::PREDICATE_LB;
        }
        else if(predicate == SETTINGS::PREDICATE_AUX && n == 2){
            // __aux__(ID, TYPE)
            aggregates[from_symbol_to_string(args[0])].type = from_symbol_to_string(args[1]);
        }
    }

   
    for(auto& [id, a]: aggregates){
        if(a.elements.empty()) continue;

        const bool eo = a.type == "ge_eo" || a.type == "le_eo";
        NormalizeResult normalizeResult = normalize(p, id, eo, a.elements);
        std::vector<ProgramGroup> groups = normalizeResult.groups; 
        // PBConstraint's literals and weights stay empty: the aggregate is described by groups only (see AmosumConstraint)
        fiore::Vector<const fiore::Literal*>* v = new fiore::Vector<const fiore::Literal*>();
        fiore::UMNWeightFunction* w = normalizeResult.w; 
        p->pbWeightFunctions.emplace_back(w);
        p->pbLiterals.emplace_back(v);
        AmosumConstraint* c = new AmosumConstraint(p, a.bound, v, w);
        c->id = id;
        c->type = a.type;
        c->ge = a.ge;
        c->groupsAtoms = std::move(a.groupsAtoms);
        c->groups = std::move(groups);
        if(c->type == "amomaximize"){
            if(p->amomaximizeConstraint != nullptr) fiore::setExitCode(fiore::CONSTANTS::ERROR_CODE, true, "At most one amomaximize statement can be declared!");

            p->amomaximizeConstraint = c;
        }

        LOG(fiore::LogLevel::DEBUG, "AmosumConstraint: ", c->toString());
        p->pBConstraints.emplace_back(c);
        p->amosumConstraints.emplace_back(c);
    }

    return p;
}

void AmosumClingoAssignment::initAmosum(const clingo_model_t* model) {
    amosumCost = 0;
    const AmosumProgram* p = static_cast<const AmosumProgram*>(this->program);
    const AmosumConstraint* amomaximize = p->amomaximizeConstraint;
    if(amomaximize == nullptr) return;

    // at most one true member per group: the weight of the first true member is the group's value
    for(const ProgramGroup& g: amomaximize->groups){
        for(const auto& [literal, weight]: g.members){
            bool isTrue;
            handleError(clingo_model_is_true(model, literal->id, &isTrue));
            if(isTrue){
                amosumCost += weight;
                break;
            }
        }
    }
}