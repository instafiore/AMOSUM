#include "amosum.h"
#include <iostream>
#include <utility>
#include <cassert>
#include "prop_clingo/propagator_clingo_c/propagator_clingo.h"




const std::vector<clingo_literal_t> AmoSumPropagator::simplifyAtLevelZero(const bool& delete_lits=false){ 

        std::string error_string = ge ? (std::to_string(_mps) + " < " + std::to_string(lb) + " !!!") : (std::to_string(_mps) + " > " + std::to_string(ub) + " !!!");
        if ((ge && _mps < lb) || (!ge && _mps > ub)) {
                debugf(error_string)
                return {PropagatorClingo::BOTTOM};
        }

        assert(!mps_violated);

        update_lazy_propagation();
        const std::vector<clingo_literal_t>* prop_from_facts = lazy_condition ? propagation_phase(nullptr, this) : nullptr;
        
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
        if(prop_from_facts) extend_vector(propagated_at_level_0, *(prop_from_facts));
        extend_vector(propagated_at_level_0, groups_literals);

        return propagated_at_level_0; 

};

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
    
    const std::vector<clingo_literal_t>* propagated = next_phase ? propagation_phase(G, this) : nullptr;
    return propagated ;
}

void AmoSumPropagator::update_lazy_propagation() {
        float p;
        if (ge) {
            mps_violated = _mps < lb;
            p = bound / static_cast<float>(_mps);
        } else {
            mps_violated = _mps > ub;
            p = _mps / static_cast<float>(bound);
        }

        lazy_condition = p >= AmoSumPropagator::LAZY_PERC ;
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
            to_be_propagated->set(l, false);
            G = group->get(l);
            G->decrease_und();
            true_group->set(G,l);
            w_p = weight->get(m_w(G, ge));
            w_n = weight->get(l);
            tg = true;
            current_sum += w_n;
        } else if (aggregate->get(not_(l))) {
            to_be_propagated->set(not_(l), false);
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

    
        G = (choice_cons == "EO") ? G : nullptr;
        bool current_sum_condition = !ge || current_sum < bound;
        bool next_phase = current_sum_condition && (w_p != w_n || amo_condition) && lazy_condition;

        return {next_phase, G};
}


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
        // clingo_literal_t ml = m_w(g, ge);
        // if (ml != l) return {_mps, SETTINGS::NONE, ml};
        auto [sml_g, ml_g] = g->update(I, ge, false, false, SETTINGS::NONE);
        int mw_g = weight->get(ml_g);

        if (ml_g != l) return {_mps, sml_g, ml_g};
        int mps_h = _mps - mw_g + weight->get(sml_g);
        return {mps_h, sml_g, ml_g};
    
    }
}

const std::vector<clingo_literal_t>* AmoSumPropagator::getReasonForLiteral(const clingo_literal_t& lit){

    auto reason_ptr = reason->get(lit) ;
    if(reason_ptr == nullptr) return nullptr ;
    std::vector<clingo_literal_t>& R = *reason_ptr;

    std::unordered_set<clingo_literal_t>* rl = redundant_lits->get(lit) ;

    bool removed = false ;
    if(rl != nullptr && rl->size() > 0){
        removed  = true ; 
        remove_elements(R, *rl);
        rl->clear();
    }

    print_reason(atomNames, R, lit, false);
    return &R; 
}


void AmoSumPropagator::compute_minimal_reason(const std::vector<clingo_literal_t>& to_minimize) {
    // Invariants: reason is grouped by self.group id, and in each self.group, literals are sorted in descending order.

    if (minimization == Minimize::NO_MINIMIZATION) {
        return;
    }

    for (auto l : to_minimize) {
        Group* g = group->get(l);
        bool derived_true = true;
        if (!g) {
            g = group->get(not_(l));
            derived_true = false;
        }
        assert(g != nullptr);

        auto [mps_h, sml_g, ml_g] = mps(g, l, !derived_true);
        int s = lb - mps_h - 1;
        auto rd = get_perfect_hash_with_pointer(redundant_lits.get(), l);

        if (minimization == Minimize::MINIMAL) {
            auto R = get_perfect_hash_with_pointer(reason.get(), l);
            if(R == nullptr) continue ;
            maximal_subset_sum_less_than_s_with_groups(derived_true, *R, s, weight, group.get(), l, I, ge, *rd);
        } else if (minimization == Minimize::CARDINALITY_MINIMAL) {
            // NOT IMPLEMENTED
        } else {
            assert(false && "Unknown minimization strategy.");
        }
    }
}




void AmoSumPropagator::onLiteralsUndefined(const std::vector<clingo_literal_t>& lits, bool wasp = true) {
    int start = wasp ? 1 : 0;

    for (size_t i = start; i < lits.size(); ++i) {
        clingo_literal_t l = lits[i];

        // Check if the literal is in the aggregate
        if (!is_in_aggregate(l)) {
            continue;
        }

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

        to_be_propagated->set(l, false);

        assert(G != nullptr);
        
        auto R = get_perfect_hash_with_pointer(reason.get(), l);
        R->clear();

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
            ge ? G->set_max(l) :  G->set_min(l); 

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
                ge ? G->set_max(l) :  G->set_min(l); 
            }
        } else {
            if ((ge && w_l >= m_weight && pos_l > pos_m) || (!ge && w_l <= m_weight && pos_l < pos_m)) {
                ge ? G->set_max(l) :  G->set_min(l); 

                if (tg == SETTINGS::NONE) {
                    _mps = _mps - m_weight + w_l;
                }
            }
        }
    }

}

void AmoSumPropagator::updated_dl(int lit, int new_dl) {
    if (new_dl != dl) {
        last_decision_lit = lit;  // Update the last decision literal if dl is different
    }
    dl = new_dl;  // Update the decision level
}

void AmoSumPropagator::add_redundant_lits(clingo_literal_t l, std::vector<clingo_literal_t> redundant_lits_vec){
    auto rd = get_perfect_hash_with_pointer(redundant_lits.get(), l);
    for(auto red_l: redundant_lits_vec){
        if(!equals(red_l, l)){
            rd->emplace(red_l);
        }
    }
}

void AmoSumPropagator::add_redundant_lit(clingo_literal_t l, clingo_literal_t redundant_l){
    auto rd = get_perfect_hash_with_pointer(redundant_lits.get(), l);
    rd->emplace(redundant_l);
}