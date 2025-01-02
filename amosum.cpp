#include "amosum.h"
#include <iostream>
#include <utility>
#include <cassert>
#include "prop_clingo/propagator_clingo_c/propagator_clingo.h"


const std::vector<clingo_literal_t> AmoSumPropagator::getLiterals(const std::vector<clingo_literal_t>& lits){
        auto start = std::chrono::high_resolution_clock::now();
        N = lits[0] + 1;
        
        minimization = get_map(params, std::string("min_r"), std::string(Minimize::NO_MINIMIZATION)) ;
        strategy = get_map(params, std::string("strategy"), strategy);
        N = lits[0] + 1;
        I.reset(new InterpretationFunction(N));
        weight.reset(new WeightFunction(N));
        group.reset(new GroupFunction(N));
        aggregate.reset(new AggregateFunction(N));
        reason_trues.reset(new PerfectHash<std::vector<clingo_literal_t>*> (N, nullptr));
        redundant_lits.reset(new PerfectHash<std::unordered_set<clingo_literal_t>*> (N, nullptr));
        _mps = 0;
        ID = get_map(params, std::string("id"), std::string("0"));
        groups.clear();  // Initialize as empty
        assumptions = get_map(params, std::string("ass"), SETTINGS::NONE_STR);
        current_sum = 0;
        lazy_prop_activated = get_map(params, std::string("lazy"), SETTINGS::FALSE_STR) == SETTINGS::TRUE_STR;
        lazy_condition = !lazy_prop_activated;

        std::string lazy_perc_str = lazy_prop_activated ? " lazy threshold " + std::to_string(AmoSumPropagator::LAZY_PERC) : SETTINGS::NONE_STR;
        debugf("Starting propagator with param ",unordered_map_to_string(params), lazy_perc_str);
        // debug("N: ",N);
        std::unordered_map<std::string, std::vector<clingo_literal_t>> groups_raw;

        lb = SETTINGS::NONE;
        ub = SETTINGS::NONE;
        std::vector<clingo_literal_t> bind;
        std::regex negative_lit_regex("^not\\s+([\\w()]+)");
        std::string bound_str = this->ge ? SETTINGS::PREDICATE_LB : SETTINGS::PREDICATE_UB;
       

        std::unordered_map<std::string,int> weights_names ; 

        ID = get_map(params, std::string("id"), std::string("0"));

        int count = 0 ;
        
        std::chrono::duration<double> whole_first_for_inner(0.0);
        for(auto &[symbolic_atom, literal]: atomNames){
            std::chrono::time_point<std::chrono::high_resolution_clock> start;
            std::chrono::time_point<std::chrono::high_resolution_clock> end(start);
            
            std::string a = from_symbol_to_string(symbolic_atom);
            if (a.length() > SETTINGS::PREDICATE_GROUP.length() and a.substr(0, SETTINGS::PREDICATE_GROUP.length() + 1) == SETTINGS::PREDICATE_GROUP + "(") {
                    start = std::chrono::high_resolution_clock::now();
                      
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
                     
                    // if (std::regex_match(lit_str, match, negative_lit_regex)) {
                    //         atom_name = match[1].str(); 
                    //         lit = atomNames[from_string_to_symbol(atom_name, atomNames)];
                    //         lit = not_(lit);
                    // }else{
                    //         lit = atomNames[from_string_to_symbol(atom_name, atomNames)];
                    // }
                    end = std::chrono::high_resolution_clock::now();     
                    lit = atomNames[from_string_to_symbol(atom_name, atomNames)];
                    
            
                    std::string plus_str = from_symbol_to_string(terms[1]);
                    bool plus_bool = plus_str == "\"+\""  ;
                    int sign = plus_bool ? 1 : -1 ;
                    lit *= sign;

                    int w = std::stoi(from_symbol_to_string(terms[2]));
                    weight->set(lit, w) ; 
                    weights_names[lit_str] = w ;
                    std::string group_id = from_symbol_to_string(terms[3]);

                    std::vector<clingo_literal_t> G = get_map_value_vector(groups_raw, group_id);
                    G.push_back(lit);
                    groups_raw[group_id] = G ;
                    aggregate->set(lit, true);

                    bind.push_back(lit);
                    bind.push_back(not_(lit));
                    // debug("group:",a," atom_name: ",atom_name, " weight: ", weight->get(lit), " group_id: ", group_id, " sign: ",sign);
            }else if(a.length() > bound_str.length() and a.substr(0, bound_str.length() + 1) == bound_str + "("){
                    clingo_symbol_t const *terms;
                    size_t terms_size ;
                    handle_error(clingo_symbol_arguments(symbolic_atom, &terms, &terms_size));
                    std::string id_str = from_symbol_to_string(terms[1]);
                    if (terms_size != 2 or id_str != ID) continue ;
                    
                    if(bound != SETTINGS::NONE) assert(false) ;
                    bound = (ge ? lb = std::stoi(from_symbol_to_string(terms[0])) : ub = std::stoi(from_symbol_to_string(terms[0])));
            }
            
            std::chrono::duration<double> elapsed = end - start;
            whole_first_for_inner += elapsed;

            ++count;
        } 
        debugf("count first for getLiterals: ", count);
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> elapsed = end - start;
        debugf("whole_first_for_inner mean time: ",whole_first_for_inner.count()/count, "s");
        debugf("whole_first_for_inner time: ",whole_first_for_inner.count(), "s");
        debugf("getLiterals [inner] time: ",elapsed.count(), "s");
     
        
        // debug("bound: ",bound);
        
        weights_names_log(ID, weights_names);

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
        // debug("lits: ", vector_to_string(lits));
        for (size_t i = 1; i < lits.size(); ++i) { // Start from index 1
            clingo_literal_t l = lits[i];
            try {
                update_phase(l, 0); 
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

void AmoSumPropagator::update_lazy_propagation() {
        float p;
        if (ge) {
            mps_violated = _mps < lb;
            p = bound / _mps;
        } else {
            mps_violated = _mps > ub;
            p = _mps / bound;
        }

        lazy_condition = p >= AmoSumPropagator::LAZY_PERC;
        if (mps_violated) {
            lazy_condition = true;
        }

        if (!lazy_prop_activated) {
            lazy_condition = true; // Forcing to not be lazy
        }
}

std::pair<bool, Group*> AmoSumPropagator::update_phase(clingo_literal_t l, int dl = 0) {
       
        int w_p = 0;
        int w_n = 0;
        I->set(l, true);
        bool tg = false;
        Group* G = nullptr;
        mps_violated = false;
        ++count;

        bool amo_condition = false;
        if (aggregate->get(l)) {
            G = group->get(l);
            G->decrease_und();
            true_group->set(G,l);
            w_p = weight->get(m_w(G, ge));
            w_n = weight->get(l);
            tg = true;
            current_sum += w_n;
        } else if (aggregate->get(not_(l))) {
            G = group->get(not_(l));
            G->decrease_und();
            auto [new_lit, prev] = G->update(I, ge, false, false, l);
            if (not_(l) == prev) {
                G->set_max_min(new_lit, ge);
                if (true_group->get(G) == SETTINGS::NONE) { 
                    w_n = weight->get(new_lit);
                    w_p = weight->get(prev);
                }
                if (choice_cons == "AMO") {
                    amo_condition = true;
                }
            } else if (not_(l) != new_lit) {
                return {false, nullptr};
            } else if (choice_cons == "AMO") {
                amo_condition = true;
            } else {
                return {false, nullptr};
            }
        } else {
            return {false, nullptr};
        }

        _mps = _mps - w_p + w_n;
        update_lazy_propagation();

        if(lazy_condition and count % 10000 == 0) debugf("[mps: ",_mps,", id: ",ID,"] iteration: ",count,"");

    
        G = (choice_cons == "EO") ? G : nullptr;
        bool current_sum_condition = !ge || current_sum < bound;
        bool next_phase = current_sum_condition && (w_p != w_n || amo_condition) && lazy_condition;

        return {next_phase, G};
}

const std::vector<clingo_literal_t> AmoSumPropagator::simplifyAtLevelZero(const bool& delete_lits=false){ 
        debug("_mps: ",_mps)
        std::string error_string = ge ? (std::to_string(_mps) + " < " + std::to_string(lb) + " !!!") : (std::to_string(_mps) + " > " + std::to_string(ub) + " !!!");
        if ((ge && _mps < lb) || (!ge && _mps > ub)) {
                debugf(error_string)
                return {PropagatorClingo::BOTTOM};
        }

        assert(!mps_violated);

        update_lazy_propagation();
        const std::vector<clingo_literal_t>* prop_from_facts = lazy_condition ? propagation_phase(nullptr, this) : new std::vector<clingo_literal_t>();
        
        if (delete_lits) {
                simplifyLiterals(facts, aggregate.get(), group.get(), ge, I); 
        }

        std::vector<std::string> assumptions_vec ;
        if (!assumptions.empty()) {
            assumptions_vec = convert_assparam_to_assarray(assumptions);
        }

        std::vector<clingo_literal_t> propagated_at_level_0 ;
        std::vector<clingo_literal_t> assumption_literals = create_assumptions_lits(assumptions_vec, atomNames);
        extend_vector(propagated_at_level_0, assumption_literals);
        extend_vector(propagated_at_level_0, *(prop_from_facts));
        extend_vector(propagated_at_level_0, groups_literals);

        return propagated_at_level_0; 

};

std::tuple<int, clingo_literal_t, clingo_literal_t> AmoSumPropagator::mps(Group* g, clingo_literal_t l, bool assumed) {
    if (assumed) {
        clingo_literal_t ml_g = m_w(g, ge);
        int mw_g = weight->get(ml_g);

        // Ensure true_group[g] is not set
        assert(true_group->get(g) == SETTINGS::NONE);

        int mps_h = _mps - mw_g + weight->get(l);
        return {mps_h, l, ml_g};
    } else {
        assert(true_group->get(g) == SETTINGS::NONE);
        auto [sml_g, ml_g] = g->update(I, ge, false, false, SETTINGS::NONE);
        int mw_g = weight->get(ml_g);

        if (ml_g != l) return {_mps, sml_g, ml_g};
        int mps_h = _mps - mw_g + weight->get(sml_g);
        return {mps_h, sml_g, ml_g};
    
    }
}

const std::vector<clingo_literal_t>* AmoSumPropagator::getReasonForLiteral(const clingo_literal_t& lit){

    std::vector<clingo_literal_t>* rt = reason_trues->get(lit);
    std::vector<clingo_literal_t> &R = rt != nullptr and rt->size() > 0 ? *rt : reason ;
    if(rt != nullptr and rt->size() > 0) extend_vector(R, reason);

    std::unordered_set<clingo_literal_t>* rl = redundant_lits->get(lit) ;

    bool removed = false ;

    if(rl != nullptr && rl->size() > 0){
        removed  = true ; 
        remove_elements(R, *rl);
    }

    print_reason(atomNames, R, lit, false);

    return &R; 
}

void AmoSumPropagator::updated_dl(int lit, int new_dl) {
    if (new_dl != dl) {
        last_decision_lit = lit;  // Update the last decision literal if dl is different
    }
    dl = new_dl;  // Update the decision level
}


const std::vector<clingo_literal_t>* AmoSumPropagator::onLiteralTrue(const clingo_literal_t& lit, const int& dl){

    if(!is_in_aggregate(lit))   return nullptr ;

    updated_dl(lit, dl);
    current_literal = lit ;

    if(I->get(lit) == true) return nullptr ; // If lit is already true then no progation will take place

    assert(I->get(lit) != false);

    auto [next_phase, G] = update_phase(lit, dl);

    if (dl == 0) {
        std::vector<clingo_literal_t> singleton = {lit} ;
        simplifyLiterals(singleton, aggregate.get(), group.get(), ge, I); 
    }
    
    return next_phase ? propagation_phase(G, this) : nullptr;
}


void AmoSumPropagator::onLiteralsUndefined(const std::vector<clingo_literal_t>& lits, bool wasp = true) {
    int start = wasp ? 1 : 0;

    for (size_t i = start; i < lits.size(); ++i) {
        clingo_literal_t l = lits[i];

        // Check if the literal is in the aggregate
        if (!is_in_aggregate(l)) {
            continue;
        }

        // propagated[l] = false;

        // Handle early stop in propagation phase
        if (I->get(l) == SETTINGS::NONE) {
            continue;
        }

        // Update interpretation
        I->set(l, SETTINGS::NONE);

        // Update the group and max weight
        Group* G = group->get(l);
        if (G == nullptr) {
            G = group->get(not_(l));
            l = not_(l);
        }

        assert(G != nullptr);

        // Increase the number of undefined literals in the group
        G->increase_und();

        clingo_literal_t tg = true_group->get(G);

        // Handle the case where the true literal becomes undefined
        int w_l = weight->get(l);
        if (tg == l) {
            true_group->set(G, SETTINGS::NONE);
            current_sum -= w_l;
        }

        clingo_literal_t m_und = m_w(G, ge);

        if (m_und == SETTINGS::NONE) {
            if (ge) {
                G->set_max(l);
            } else {
                G->set_min(l);
            }

            if (tg == SETTINGS::NONE) {
                if (choice_cons == "AMO") {
                    _mps += w_l;
                } else {
                    assert(false);
                }
            }
            continue;
        }

        int pos_m = G->ord_i[m_und];
        int pos_l = G->ord_i[l];
        int m_weight = weight->get(m_und);

        if (tg == l) {
            // Update the _mps
            if ((m_weight > w_l && ge) || (m_weight < w_l && !ge)) {
                _mps = _mps - w_l + m_weight;
            }

            // Update max or min undefined
            if ((ge && pos_m < pos_l) || (!ge && pos_m > pos_l)) {
                if (ge) {
                    G->set_max(l);
                } else {
                    G->set_min(l);
                }
            }
        } else {
            if ((ge && w_l >= m_weight && pos_l > pos_m) || (!ge && w_l <= m_weight && pos_l < pos_m)) {
                if (ge) {
                    G->set_max(l);
                } else {
                    G->set_min(l);
                }

                if (tg == SETTINGS::NONE) {
                    _mps = _mps - m_weight + w_l;
                }
            }
        }
    }

}