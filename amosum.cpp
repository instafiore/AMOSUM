#include "amosum.h"
#include <iostream>
#include <utility>
#include <cassert>


std::vector<clingo_literal_t> AmoSumPropagator::getLiterals(const std::vector<clingo_literal_t>& lits){
        N = lits[0] + 1;
        
        minimization = get_map(params, std::string("min_r"), std::string(Minimize::NO_MINIMIZATION)) ;
        strategy = get_map(params, std::string("strategy"), strategy);
        N = lits[0] + 1;
        I.reset(new InterpretationFunction(N));
        weight.reset(new WeightFunction(N));
        group.reset(new GroupFunction(N));
        aggregate.reset(new AggregateFunction(N));
        reason_trues.reset(new PerfectHash<vector_lit_ptr> (N, nullptr));
        redundant_lits.reset(new PerfectHash<vector_lit_ptr> (N, nullptr));
        _mps = 0;
        ID = get_map(params, std::string("id"), std::string("0"));
        groups.clear();  // Initialize as empty
        assumptions = get_map(params, std::string("ass"), std::string(""));
        current_sum = 0;
        lazy_prop_actived = get_map(params, std::string("lazy"), SETTINGS::FALSE_STR) == SETTINGS::TRUE_STR;
        lazy_condition = !lazy_prop_actived;

        std::string lazy_perc_str = lazy_prop_actived ? " lazy threshold " + std::to_string(AmoSumPropagator::LAZY_PERC) : "";
        debugf("Starting propagator with param ",unordered_map_to_string(params), lazy_perc_str);
        // debug("N: ",N);
        std::unordered_map<std::string, std::vector<clingo_literal_t>> groups_raw;

        lb = SETTINGS::NONE;
        ub = SETTINGS::NONE;
        std::vector<clingo_literal_t> bind;
        std::regex negative_lit_regex("^not\\s+([\\w()]+)");
        std::string bound_str = this->ge ? SETTINGS::PREDICATE_LB : SETTINGS::PREDICATE_UB;
        auto bound = SETTINGS::NONE;

        std::unordered_map<std::string,std::string> weights_names ; 

        ID = get_map(params, std::string("id"), std::string("0"));
     
        for(auto &[symbolic_atom, literal]: atomsNames){
                std::string a = from_symbol_to_string(symbolic_atom);
                if (a.length() > SETTINGS::PREDICATE_GROUP.length() and a.substr(0, SETTINGS::PREDICATE_GROUP.length() + 1) == SETTINGS::PREDICATE_GROUP + "(") {
                        clingo_literal_t group_literal = literal ;
                        clingo_symbol_t const *terms;
                        size_t terms_size ;
                        handle_error(clingo_symbol_arguments(symbolic_atom, &terms, &terms_size));

                        if (terms_size != 5) continue;

                        std::string id_str = from_symbol_to_string(terms[4]);
                        if ( id_str != ID) {
                                continue;
                        };

                        groups_literals.push_back(not_(group_literal));
                        std::string lit_str = from_symbol_to_string(terms[0]);
                        std::string atom_name = lit_str ;
                        clingo_literal_t lit ;
                        std::smatch match;
                        if (std::regex_match(lit_str, match, negative_lit_regex)) {
                                atom_name = match[1].str(); 
                                lit = atomsNames[from_string_to_symbol(atom_name, atomsNames)];
                                lit = not_(lit);
                        }else{
                                lit = atomsNames[from_string_to_symbol(atom_name, atomsNames)];
                        }

                
                        std::string plus_str = from_symbol_to_string(terms[1]);
                        bool plus_bool = plus_str == "\"+\""  ;
                        int sign = plus_bool ? 1 : -1 ;
                        lit *= sign;

                        int w = std::stoi(from_symbol_to_string(terms[2]));
                        weight->set(lit, w) ; 
                        weights_names[atom_name] = w ;
                        std::string group_id = from_symbol_to_string(terms[3]);

                        std::vector<clingo_literal_t> G = get_map_value_vector(groups_raw, group_id);
                        G.push_back(lit);
                        groups_raw[group_id] = G ;
                        aggregate->set(lit, true);
                
                        bind.push_back(lit);
                        bind.push_back(not_(lit));
                        // debug("group:",a," atom_name: ",atom_name, " weight: ", weight->get(lit), " group_id: ", group_id);
                        
                }else if(a.length() > bound_str.length() and a.substr(0, bound_str.length() + 1) == bound_str + "("){
                        clingo_symbol_t const *terms;
                        size_t terms_size ;
                        handle_error(clingo_symbol_arguments(symbolic_atom, &terms, &terms_size));
                        std::string id_str = from_symbol_to_string(terms[1]);
                        if (terms_size != 2 or id_str != ID) continue ;
                        
                        if(bound != SETTINGS::NONE) assert(false) ;
                        bound = (ge ? lb = std::stoi(from_symbol_to_string(terms[0])) : ub = std::stoi(from_symbol_to_string(terms[0])));
                }
        } 
        // debug("bound: ",bound);

        for(auto &[group_id, lits_group]: groups_raw){
                std::vector<std::pair<int, int>> lits_ord;
                for (int lit : lits_group) lits_ord.emplace_back(lit, weight->get(lit));
                std::sort(lits_ord.begin(), lits_ord.end(), [](const std::pair<int, int>& a, const std::pair<int, int>& b) {
                        return a.second < b.second; 
                });
                std::vector<clingo_literal_t> ord_l(lits_ord.size(), SETTINGS::NONE);
                std::unordered_map<int, int> ord_i; 

                for (size_t i = 0; i < lits_ord.size(); ++i) {
                        int l = lits_ord[i].first; 
                        ord_l[i] = l;             
                        ord_i[l] = i;            
                }

                Group* G = new Group(ord_l,ord_i,group_id) ;

                _mps = _mps + weight->get(m_w(G, ge)) ;

                groups.push_back(G);
                
                for(const clingo_literal_t& lit: lits_group)  group->set(lit, G);
        }

        size_t nGroup = Group::autoincrement ;
        true_group.reset(new TrueGroupFunction(nGroup)) ;

        for (size_t i = 1; i < lits.size(); ++i) { // Start from index 1
            int l = lits[i];
            try {
                // update_phase(l); 
                inconsistent_at_level_0 = false;
            } catch (const std::exception& e) {
                inconsistent_at_level_0 = true;
                // debug("Incosistent at level 0: ",e.what())
                // break;
            }
        }

        // Set facts to literals starting from index 1
        facts.assign(lits.begin() + 1, lits.end());

        // Set class variables
        last_decision_lit = 1;
        dl = 0;

        return bind;
}
