#include "amosum_program.h"
#include <algorithm>

// Anonymus namespace
namespace {
    struct GroupElement{
        const fiore::Literal* literal;
        int64_t weight;
        std::string groupId;
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

fiore::Program* AmosumProgram::AmosumGrounder::ground(const std::vector<std::string>& ASPInputs){
    AmosumProgram* p = static_cast<AmosumProgram*>(fiore::Program::ProgramGrounder::ground(ASPInputs));
  
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
            a.elements.push_back({lit, w, from_symbol_to_string(args[3])});
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

    const size_t N = p->maxLiteralId + 1;
    for(auto& [id, a]: aggregates){
        if(a.elements.empty()) continue;

        // weights must be set before creating the constraint: PBConstraint's constructor preprocesses them
        fiore::Vector<const fiore::Literal*>* literals = new fiore::Vector<const fiore::Literal*>();
        fiore::UMNWeightFunction* weights = new fiore::UMNWeightFunction(N);
        std::map<std::string, std::vector<std::pair<const fiore::Literal*, int64_t>>> groupsRaw;
        for(const GroupElement& e: a.elements){
            literals->add(e.literal);
            weights->set(e.literal->id, e.weight);
            groupsRaw[e.groupId].emplace_back(e.literal, e.weight);
        }

        AmosumConstraint* c = new AmosumConstraint(p, a.bound, literals, weights);
        c->id = id;
        c->type = a.type;
        c->ge = a.ge;
        c->groupsAtoms = std::move(a.groupsAtoms);
        c->groupFunction.reset(new fiore::PerfectNHash<clingo_literal_t, size_t>(N, fiore::CONSTANTS::NONE64));

        for(auto& [groupId, lits]: groupsRaw){
            std::sort(lits.begin(), lits.end(), [](const auto& x, const auto& y){
                return x.second != y.second ? x.second < y.second : x.first->id < y.first->id;
            });
            std::vector<clingo_literal_t> ord_l;
            std::unordered_map<clingo_literal_t, size_t> ord_i;
            for(const auto& [l, w]: lits){
                ord_i[l->id] = ord_l.size();
                ord_l.push_back(l->id);
                c->groupFunction->set(l->id, c->groups.size());
            }
            c->groups.push_back(new Group(std::move(ord_l), std::move(ord_i), groupId));
        }

        LOG(fiore::LogLevel::DEBUG, "AmosumConstraint: ", c->toString());
        p->pBConstraints.emplace_back(c);
        p->amosumConstraints.emplace_back(c);
    }

    return p;
}