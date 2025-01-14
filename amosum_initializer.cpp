#pragma once
#include "amosum_initializer.h"


const std::string AmoSumInitializer::DEFAULT_LAZY = std::string("dynamic");

const std::vector<clingo_literal_t> AmoSumInitializer::getLiterals(const std::vector<clingo_literal_t>& lits, AmoSumPropagator* amosum_propagator){

        auto start = std::chrono::high_resolution_clock::now();
        amosum_propagator->N = lits[0] + 1;
        amosum_propagator->minimization = get_map(amosum_propagator->params, std::string("min_r"), std::string(Minimize::NO_MINIMIZATION)) ;
        amosum_propagator->strategy = get_map(amosum_propagator->params, std::string("strategy"), amosum_propagator->strategy);
        amosum_propagator->I.reset(new InterpretationFunction(amosum_propagator->N));
        amosum_propagator->group.reset(new GroupFunction(amosum_propagator->N));

        #ifdef PRIVATE_REASON
        amosum_propagator->reason.reset(new PerfectHash<std::vector<clingo_literal_t>*> (amosum_propagator->N, nullptr));
        #else
        amosum_propagator->reason_trues.reset(new PerfectHash<std::vector<clingo_literal_t>*> (amosum_propagator->N, nullptr));
        #endif

        amosum_propagator->redundant_lits.reset(new PerfectHash<std::unordered_set<clingo_literal_t>*> (amosum_propagator->N, nullptr));
        amosum_propagator->to_be_propagated.reset(new PerfectHash<bool>(amosum_propagator->N, false));
        amosum_propagator->_mps = 0;
        amosum_propagator->ID = get_map(amosum_propagator->params, std::string("id"), std::string("0"));
        amosum_propagator->groups.clear();  // Initialize as empty
        amosum_propagator->assumptions = get_map(amosum_propagator->params, std::string("ass"), SETTINGS::NONE_STR);
        amosum_propagator->current_sum = 0;
        

        amosum_propagator->lb = SETTINGS::NONE;
        amosum_propagator->ub = SETTINGS::NONE;


        if(first){
            weight.reset(new WeightFunction(amosum_propagator->N));
            first = false ;
            common_phase(amosum_propagator);
        }
        assign(amosum_propagator);
        specific_phase(lits, amosum_propagator);
        
        generic_data* gd = nullptr ;
        gd = get_map(generic_data_map, amosum_propagator->ID, gd);
        return gd->bind;
        
    }

    void AmoSumInitializer::common_phase(AmoSumPropagator* amosum_propagator){
        
        atomNamesString = create_atomNames_string(amosum_propagator->atomNames);
        debugf("N: ", amosum_propagator->N);

        for(auto &[symbolic_atom, literal]: *amosum_propagator->atomNames){
                
            std::string a = from_symbol_to_string(symbolic_atom);
            if (a.length() > SETTINGS::PREDICATE_GROUP.length() and a.substr(0, SETTINGS::PREDICATE_GROUP.length() + 1) == SETTINGS::PREDICATE_GROUP + "(") {
              
                    clingo_literal_t group_literal = literal ;
                    clingo_symbol_t const *terms;
                    size_t terms_size ;
                    
                    handle_error(clingo_symbol_arguments(symbolic_atom, &terms, &terms_size));
                    
                    if (terms_size != 5) continue;

                    std::string id_str = from_symbol_to_string(terms[4]);
                    amosum_propagator->groups_literals.push_back(not_(group_literal));
                    
                    std::string lit_str = from_symbol_to_string(terms[0]);
                     
                    std::string atom_name = lit_str ;
                    clingo_literal_t lit ;
                        
                    lit = atomNamesString[atom_name];
                    
                    std::string plus_str = from_symbol_to_string(terms[1]);
                    bool plus_bool = plus_str == "\"+\""  ;
                    int sign = plus_bool ? 1 : -1 ;
                    lit *= sign;

                    int w = std::stoi(from_symbol_to_string(terms[2]));
    
                    weight->set(lit, w) ; 
            
                    std::string group_id = from_symbol_to_string(terms[3]);
                    
                    
                    generic_data* gd = nullptr ;
                    gd = get_map(generic_data_map, id_str, gd);
                    if(!gd){ 
                        gd = new generic_data();
                        generic_data_map[id_str] = gd ;
                    }
                    gd->weights_names[lit_str] = w ;
                    std::vector<clingo_literal_t> G = get_map_value_vector(gd->groups_raw, group_id);
                    G.push_back(lit);
                    gd->groups_raw[group_id] = G ;

                    AggregateFunction* aggregate = nullptr ;
                    aggregate = get_map(aggregate_map, id_str, aggregate);
                    if(!aggregate){ 
                        aggregate = new AggregateFunction(amosum_propagator->N);
                        aggregate_map[id_str] = aggregate ;
                    }
                    aggregate->set(lit, true);

                    gd->bind.push_back(lit);
                    gd->bind.push_back(not_(lit));
                    
            }else if((a.length() > SETTINGS::PREDICATE_LB.length() and a.substr(0, SETTINGS::PREDICATE_LB.length() + 1) == SETTINGS::PREDICATE_LB + "(") || 
                     (a.length() > SETTINGS::PREDICATE_UB.length() and a.substr(0, SETTINGS::PREDICATE_UB.length() + 1) == SETTINGS::PREDICATE_UB + "(")) {
                    clingo_symbol_t const *terms;
                    size_t terms_size ;
                    handle_error(clingo_symbol_arguments(symbolic_atom, &terms, &terms_size));
                    std::string id_str = from_symbol_to_string(terms[1]);
                    generic_data* gd = nullptr ;
                    gd = get_map(generic_data_map, id_str, gd);
                    if(!gd){ 
                        gd = new generic_data();
                        generic_data_map[id_str] = gd ;
                    }
                    if(gd->bound != SETTINGS::NONE) assert(false) ;
                    gd->bound = std::stoi(from_symbol_to_string(terms[0]));
                    
            }
            
        }
        
    }

    void AmoSumInitializer::assign(AmoSumPropagator* amosum_propagator){
      
        std::string ID = amosum_propagator->ID ;
        amosum_propagator->aggregate.reset(aggregate_map[ID]) ;
        amosum_propagator->weight = weight.get() ;
        amosum_propagator->ge ? amosum_propagator->lb = generic_data_map[ID]->bound : amosum_propagator->ub = generic_data_map[ID]->bound ;
        amosum_propagator->bound = generic_data_map[ID]->bound ;
    }


    void AmoSumInitializer::specific_phase(const std::vector<clingo_literal_t>& lits, AmoSumPropagator* amosum_propagator){
        int max_diff = 0 ;
        std::string ID = amosum_propagator->ID ;
        std::string lazy_param = get_map(amosum_propagator->params, std::string("lazy"), DEFAULT_LAZY) ;
        amosum_propagator->lazy_prop_activated = lazy_param != SETTINGS::FALSE_STR;
        bool lazy_dynamic = lazy_param == "dynamic" ;

        #ifdef CHECK_MPS
        weights_names_log(ID, generic_data_map[ID]->weights_names);
        #endif

        for(auto &[group_id, lits_group]: generic_data_map[ID]->groups_raw){
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

                int max_w = weight->get(m_w(G, amosum_propagator->ge));
                int min_w = amosum_propagator->lazy_prop_activated ? weight->get(m_w(G, !amosum_propagator->ge)) : 0 ;


                amosum_propagator->_mps = amosum_propagator->_mps + max_w;

                int diff = std::abs(max_w - min_w) ;
                
                if (max_diff < diff)  max_diff = diff ;

                amosum_propagator->groups.push_back(G);
                
                for(const clingo_literal_t& lit: lits_group)  amosum_propagator->group->set(lit, G);
        }

        size_t nGroup = Group::autoincrement ;
        amosum_propagator->true_group.reset(new TrueGroupFunction(nGroup)) ;
        
        for (size_t i = 1; i < lits.size(); ++i) { // Start from index 1
            clingo_literal_t l = lits[i];
            try {
                amosum_propagator->update_phase(l, 0); 
                amosum_propagator->inconsistent_at_level_0 = false;
            } catch (const std::exception& e) {
                amosum_propagator->inconsistent_at_level_0 = true;
            }
        }

        // debugf("max_diff: ", max_diff); 

        amosum_propagator->LAZY_PERC = amosum_propagator->lazy_prop_activated && lazy_param != SETTINGS::TRUE_STR && !lazy_dynamic ? std::stof(lazy_param) : amosum_propagator->LAZY_PERC ;
        amosum_propagator->lazy_condition = !amosum_propagator->lazy_prop_activated;
        if(lazy_dynamic) amosum_propagator->LAZY_PERC = amosum_propagator->ge ? amosum_propagator->lb / static_cast<float>(amosum_propagator->lb + max_diff) :  (amosum_propagator->ub - max_diff) / static_cast<float>(amosum_propagator->ub);
        std::string lazy_perc_str = amosum_propagator->lazy_prop_activated ? " lazy threshold " + std::to_string(amosum_propagator->LAZY_PERC) : SETTINGS::NONE_STR;
        debugf("Starting propagator with param ",unordered_map_to_string(amosum_propagator->params), lazy_perc_str);

        // debugf("Intial mps of ",ID ,"is: ", _mps);

        // Set facts to literals starting from index 1
        amosum_propagator->facts.assign(lits.begin() + 1, lits.end());

        // Set class variables
        amosum_propagator->last_decision_lit = 1;
        amosum_propagator->dl = 0;
    }

AmoSumInitializer* AmoSumInitializer::instance = nullptr;