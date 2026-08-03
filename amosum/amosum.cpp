#include "amosum.h"
#include <iostream>
#include <utility>
#include <cassert>
#include "amoclingo/propagator_clingo_c/propagator_clingo.h"
#include "settings.h"


void AmoSumPropagator::resetPropagator(){
        this->last_decision_lit = 1;
        this->dl = 0;
        // this->true_group->reset();
        // this->I->reset();
        // this->reason->reset();
        // this->redundant_lits->reset();
        // this->current_sum = 0 ;
    }

void AmoSumPropagator::updateBound(int bound){
    this->bound = bound;
    // printf("Updating bound AmoSumPropagator with %d\n", this->bound);
    this->ge ? this->lb = bound : this->ub = bound ;
    
}

const std::vector<clingo_literal_t> AmoSumPropagator::simplifyAtLevelZero(const bool& delete_lits=false){ 

        // debug_old("simplifyAtLevelZero for ", unordered_map_to_string(params), " with mps: ",_mps," and bound: ",bound);
        debug(DEBUG, "simplifyAtLevelZero for ", unordered_map_to_string(params), " with mps: ",mps()," and bound: ",bound);

        
        std::string error_string = ge ? (std::to_string(mps()) + " < " + std::to_string(lb) + " !!!") : (std::to_string(mps()) + " > " + std::to_string(ub) + " !!!");
        if ((ge && mps() < lb) || (!ge && mps() > ub)) {
                // debugf_old(error_string)
                debug(ERROR, error_string);
                return {SETTINGS::PLITBOTTOM};
        }
        
        
        assert(!sum_violated);

        
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
            sum_violated = mps() < lb;
            p = bound / static_cast<float>(mps());
        } else {
            sum_violated = mps() > ub;
            p = mps() / static_cast<float>(bound);
        }

        lazy_condition = p >= this->lazy_perc;
        if (sum_violated) {
            lazy_condition = true;
        }

}

std::pair<bool, Group*> AmoSumPropagator::update_phase(clingo_literal_t l, int dl = 0) {
       
        int w_p = 0;
        int w_n = 0;
        I->set(l, true);
        bool tg = false;
        Group* G = nullptr;
        sum_violated = false;
        ++count;

        bool amo_condition = false;
        bool sumChanged = false;
        bool insideAggr = aggregate->get(l) ;
        bool amo_le = constraint == "AMO" && !ge;


        if (insideAggr) {
            
            G = group->get(l);
            G->decrease_und();
            true_group->setTrueLiteral(G,l);
            w_p = weight->get(m_w(G, ge));
            w_n = weight->get(l);
            tg = true;
            current_sum += w_n;
            if(amo_le) sumChanged = w_n > 0;

            if(G->count_undef == 0){
                if(l == G->max_und()) G->set_max(SETTINGS::NONE);
                if(l == G->min_und()) G->set_min(SETTINGS::NONE);
            }else{

            }
            
        } else if (aggregate->get(not_(l))) {
            G = group->get(not_(l));
            G->decrease_und();
            // auto [new_lit, prev] = G->update(I, ge, false, false, l);
            // --MPC--
            auto [second_max_und, max_und] = G->update(I, dynamicMPC ? true: ge, false, false, l);
            clingo_literal_t second_min_und = 0;
            clingo_literal_t min_und = 0;
            if(dynamicMPC){ 
                std::pair<clingo_literal_t, clingo_literal_t> updatePair = G->update(I, false, false, false, l);
                second_min_und = updatePair.first;
                min_und = updatePair.second;
            }
            // --MPC--


            // if (not_(l) == prev) {
            //     G->set_max_min(new_lit, ge);
            //     if (true_group->getTrueLiteral(G) == SETTINGS::NONE) { 
            //         w_n = weight->get(new_lit);
            //         w_p = weight->get(prev);
            //     }
            //     if (constraint == "AMO") {
            //         amo_condition = true;
            //     }
            // } else if (not_(l) != new_lit) {
            //     return {false, nullptr};
            // } else if (constraint == "AMO") {
            //     amo_condition = true;
            // } else {
            //     return {false, nullptr};
            // }

            bool ge_or_staticMPC = ge || !dynamicMPC;
            bool affects_second_max = (ge_or_staticMPC && not_(l) == second_max_und) || (!ge_or_staticMPC && not_(l) == second_min_und);
            // --MPC--
            if (not_(l) == max_und || (dynamicMPC && not_(l) == min_und)) {
                if(not_(l) == max_und) (dynamicMPC ? G->set_max(second_max_und): G->set_max_min(second_max_und, ge));
                if(dynamicMPC && not_(l) == min_und) G->set_min(second_min_und);
                bool affect_mps = !dynamicMPC || (ge && not_(l) == max_und) || (!ge && not_(l) == min_und);
                // if (affect_mps && true_group->getTrueLiteral(G) == SETTINGS::NONE) { 
                //     w_n = weight->get(ge || !dynamicMPC ? second_max_und: second_min_und);
                //     w_p = weight->get(ge || !dynamicMPC ? max_und: min_und);
                //     amo_condition = true;
                // }else if(true_group->getTrueLiteral(G) == SETTINGS::NONE && affects_second_max && constraint == "AMO"){
                //     amo_condition = true;
                // }

                if(true_group->getTrueLiteral(G) == SETTINGS::NONE){
                    if((affect_mps || affects_second_max) && constraint == "AMO")
                        amo_condition = true;
                    if(affect_mps){
                        w_n = weight->get(ge || !dynamicMPC ? second_max_und: second_min_und);
                        w_p = weight->get(ge || !dynamicMPC ? max_und: min_und);
                    }
                }

            } else if (!affects_second_max) {
                return {false, nullptr};
            } else if (constraint == "AMO") {
                amo_condition = true;
            } else {
                return {false, nullptr};
            }
            // --MPC--
        } else {
            return {false, nullptr};
        }

        _mps = _mps - w_p + w_n;

        bool mpcCondition = true; // true by default
        // --MPC-- MPC update
        if(dynamicMPC){
            if(G == mpc || G->count_undef <= 1) updateMPC();
            if(mpc != nullptr){
                size_t mpcValue = mpc->pc(ge, constraint, weight.get());
                mpcCondition =  ge ? ((size_t)mps()) - mpcValue < (size_t)this->bound : ((size_t)mps()) + mpcValue > (size_t)this->bound; 
            }else{
                mpcCondition  = false;
            }
        }
        // --MPC--
        
        update_lazy_propagation();

        if(!amo_le) sumChanged = w_p != w_n;
        G = (constraint == "EO") ? G : nullptr;
        bool current_sum_condition = !ge || current_sum < bound;
        bool next_phase = (current_sum_condition && (sumChanged || amo_condition) && lazy_condition && mpcCondition) || sum_violated;
        // bool next_phase = sum_violated || (current_sum_condition && (sumChanged || amo_condition) && lazy_condition && mpcCondition);
        // if(dl >= 5000) debugf("ID: ",ID," mps: ",_mps, " next_phase: ", next_phase, " lazy_condition: ",lazy_condition);
        return {next_phase, G};
}

void AmoSumPropagator::updateMPC() noexcept{
    mpc = nullptr;
    for(Group* group: groups){
        if(true_group->getTrueLiteral(group) != SETTINGS::NONE || group->count_undef == 0) continue;
        if(mpc == nullptr) mpc = group;
        else{
            int currentMpc = mpc->pc(ge, constraint, weight.get());
            int groupMpc = group->pc(ge, constraint, weight.get());
            if(currentMpc < groupMpc){
                mpc = group;
            }
        }  
    }
}


std::tuple<int, clingo_literal_t, clingo_literal_t> AmoSumPropagator::mpsH(clingo_literal_t l, bool assumed) {
    
    Group* g = this->group->get(l);
    bool inAggr = true;
    if(g == nullptr){
        g = this->group->get(not_(l));
        inAggr = false;
        assert(g != nullptr);
    }

    if(constraint == "AMO" && !ge){
        if(inAggr & assumed || !inAggr & !assumed) return {mps() + weight->get(l), l, SETTINGS::NONE};
        return {mps(), SETTINGS::NONE, SETTINGS::NONE};
    }
    
    if (assumed) {
        clingo_literal_t ml_g = m_w(g, ge);
        int mw_g = weight->get(ml_g);

        // Ensure true_group[g] is not set
        if(true_group->getTrueLiteral(g) != SETTINGS::NONE){
            std::string name_tr = get_name(atomNames, true_group->getTrueLiteral(g));
            std::string name_l = get_name(atomNames, l);
            // debugf_old("name_tr: ",name_tr, " name_l: ",name_l);
            debug(ERROR, "There is already a true literal in the group, name_tr: ",name_tr, " name_l: ",name_l);
            cmn::setExitCode(CONSTANTS::ERROR_CODE);
        }
        
        int mps_h = _mps - mw_g + weight->get(l);
        return {mps_h, l, ml_g};
    } else {
        assert(true_group->getTrueLiteral(g) == SETTINGS::NONE);
        // clingo_literal_t ml = m_w(g, ge);
        // if (ml != l) return {_mps, SETTINGS::NONE, ml};
        auto [sml_g, ml_g] = g->update(I, ge, false, false, SETTINGS::NONE);
        int mw_g = weight->get(ml_g);

        if (ml_g != l) return {_mps, sml_g, ml_g};
        int mps_h = _mps - mw_g + weight->get(sml_g);
        return {mps_h, sml_g, ml_g};
    
    }
}

int AmoSumPropagator::maxPossibleSum(clingo_literal_t l){
    bool lInAggr=true; // assuming l \in \lits|_s or \not l \in \lits|_s
    Group* gl = group->get(l);
    if(gl == nullptr){
        gl = group->get(not_(l));
        lInAggr=false;
    }

    int maxPossibleSum = 0;
    for (Group* g : groups){
        if(g != gl){
            // g != gl
            maxPossibleSum += weight->get(max_w(I.get(), g));
        }else{
            // g = gl
            if(lInAggr) maxPossibleSum += weight->get(l);
            else{
                int tg = true_group->getTrueLiteral(g);
                if(tg != SETTINGS::NONE) maxPossibleSum += weight->get(tg);
                else maxPossibleSum += weight->get(max_w(I.get(), g, {l}));
            }
        }
    }

    return maxPossibleSum;
}


int AmoSumPropagator::minPossibleSum(clingo_literal_t l){
    int minPossibleSum=0;
    return minPossibleSum;
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
    printReason(atomNames, R, lit);
    return &R; 
}


void AmoSumPropagator::compute_minimal_reason(const std::vector<clingo_literal_t>& to_minimize) {
    // Invariants: reason is grouped by self.group id, and in each self.group, literals are sorted in descending order.

    if (minimization == Minimize::NO_MINIMIZATION || minimization == Minimize::IJCAI || minimization == Minimize::MINIMAL_ON_THE_FLY) {
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

        auto mps_h = sum_violated ? mps() : std::get<0>(mpsH(l, !derived_true));
        int s = lb - mps_h - 1;
        auto rd = get_perfect_hash_with_pointer(redundant_lits.get(), l);
        auto R = get_perfect_hash_with_pointer(reason.get(), l);
        if(R == nullptr) continue ;
        
        if (minimization == Minimize::MINIMAL) {
            maximal_subset_sum_less_than_s_with_groups(derived_true, *R, s, weight.get(), group.get(), l, I, ge, *rd);
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


        // Update interpretation
        if (I->get(l) == SETTINGS::NONE) {
            continue;
        }

        I->set(l, SETTINGS::NONE);


        // Update the group and max weight
        Group* G = group->get(l);
        if (G == nullptr) {
            G = group->get(not_(l));
            l = not_(l);
        }


        
        assert(G != nullptr);
        
        auto R = get_perfect_hash_with_pointer(reason.get(), l);
        R->clear();

        // Increase the number of undefined literals in the group
        G->increase_und();

        clingo_literal_t tg = true_group->getTrueLiteral(G);

        // Handle the case where the true literal becomes undefined
        int w_l = weight->get(l);
        if (tg == l) {
            true_group->setTrueLiteral(G, SETTINGS::NONE);
            current_sum -= w_l;
        }


        // clingo_literal_t m_und = m_w(G, ge);
        // --MPC--
        clingo_literal_t max_und = m_w(G, dynamicMPC ? true: ge);
        clingo_literal_t min_und = 0;
        if(dynamicMPC) min_und = m_w(G, false);
        // --MPC--

        // if (m_und == SETTINGS::NONE) {
        // --MPC--
        if (max_und == SETTINGS::NONE || (dynamicMPC && min_und == SETTINGS::NONE)) {
            max_und = SETTINGS::NONE;
            min_und = SETTINGS::NONE;
            assert(!dynamicMPC || min_und == SETTINGS::NONE);
        // --MPC--
            // G->set_max_min(l, ge);
            // --MPC--
            if(dynamicMPC){
                G->set_max(l);
                G->set_min(l);
            }else{
                G->set_max_min(l, ge);
            }
            // --MPC--

            if (tg == SETTINGS::NONE) {
                if (constraint == "AMO" && ge) {
                    _mps += w_l;
                } else {
                    assert(constraint == "AMO");
                }
            }
            // --MPC-- MPC update
            if(dynamicMPC && (tg == SETTINGS::NONE || tg == l) && (mpc == nullptr || G->pc(ge, constraint, weight.get()) > mpc->pc(ge, constraint, weight.get()))) mpc = G;
            // --MPC--
            continue;
        }

        assert(max_und != SETTINGS::NONE);
        assert(!dynamicMPC || min_und != SETTINGS::NONE);

        // int pos_m = G->ord_i[m_und];
        // int pos_l = G->ord_i[l];
        // int m_weight = weight->get(m_und);

        // --MPC--
        int pos_l = G->ord_i[l];
        int pos_max = G->ord_i[max_und];
        int max_weight = weight->get(max_und);
        int pos_min = SETTINGS::NONE; 
        int min_weight =  SETTINGS::NONE;
        if(dynamicMPC){
            pos_min = G->ord_i[min_und];
            min_weight = weight->get(min_und);
        }
        // --MPC--



        if (tg == l) {
            // Update the _mps
            // if ((m_weight > w_l && ge) || (m_weight < w_l && !ge)) {
            //     _mps = _mps - w_l + m_weight;
            // }
            // --MPC--
            if (
                (((max_weight > w_l && ge) || (min_weight < w_l && !ge)) && dynamicMPC) ||
                (((max_weight > w_l && ge) || (max_weight < w_l && !ge)) && !dynamicMPC)
            ) {
                _mps = _mps - w_l + (ge || !dynamicMPC ? max_weight : min_weight);
            }
            // --MPC--

            // Update max or min undefined
            // if ((ge && pos_m < pos_l) || (!ge && pos_m > pos_l)) {
            //     ge ? G->set_max(l) :  G->set_min(l); 
            // }
            // --MPC--
            if(dynamicMPC){
                if(pos_max < pos_l) G->set_max(l);
                if(pos_min > pos_l) G->set_min(l);
            }else{
                if ((ge && pos_max < pos_l) || (!ge && pos_max > pos_l)) {
                    G->set_max_min(l, ge);
                }
            }
            // --MPC--
        } else {
            // if ((ge && w_l >= m_weight && pos_l > pos_m) || (!ge && w_l <= m_weight && pos_l < pos_m)) {
            //     ge ? G->set_max(l) :  G->set_min(l); 

            //     if (tg == SETTINGS::NONE) {
            //         _mps = _mps - m_weight + w_l;
            //     }
            // }
            // --MPC--
            if(dynamicMPC){
                if(pos_l > pos_max) G->set_max(l);
                if(pos_l < pos_min) G->set_min(l);
            }else{
                if((ge && pos_l > pos_max) || (!ge && pos_l < pos_max)) G->set_max_min(l, ge);
            }
            if (tg == SETTINGS::NONE && 
                (
                    (dynamicMPC &&  ((ge && w_l >= max_weight && pos_l > pos_max) || (!ge && w_l <= min_weight && pos_l < pos_min))) ||
                    (!dynamicMPC && ((ge && w_l >= max_weight && pos_l > pos_max) || (!ge && w_l <= max_weight && pos_l < pos_max)))
                )
            ) {
                _mps = _mps - (ge || !dynamicMPC ? max_weight : min_weight) + w_l;
            }
            // --MPC--
        }

        // --MPC-- MPC update
        if(dynamicMPC && (tg == SETTINGS::NONE || tg == l) && (mpc == nullptr || G->pc(ge, constraint, weight.get()) > mpc->pc(ge, constraint, weight.get()))) mpc = G;
        // --MPC--

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

bool AmoSumPropagator::is_true(clingo_literal_t l){
    if(dl == 0) return false;
    
    bool res = false;
    const clingo_assignment_t *assignment = clingo_propagate_control_assignment(control);
    if(solver == AmoSumPropagator::CLINGO){
        clingo_literal_t slit = (*map_plit_slit)[l];
        clingo_assignment_is_true(assignment, slit, &res);
    }else{
        assert(false);
        // NOT IMPLEMENTED
    }
    return res ;
}

bool AmoSumPropagator::is_false(clingo_literal_t l){
    if(dl == 0) return false;
    bool res = false;
    const clingo_assignment_t *assignment = clingo_propagate_control_assignment(control);
    if(solver == AmoSumPropagator::CLINGO){
        clingo_literal_t slit = (*map_plit_slit)[l];
        clingo_assignment_is_false(assignment, slit, &res);
    }else{
        // NOT IMPLEMENTED
        assert(false);
    }
    return res ;
}

bool AmoSumPropagator::is_undef(clingo_literal_t l){
    return !is_false(l) && !is_true(l) ;
}