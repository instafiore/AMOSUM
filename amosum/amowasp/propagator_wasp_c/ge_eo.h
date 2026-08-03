#pragma once
#include <clingo.h>
#include <stdlib.h>
#include <stdio.h>
#include <string>
#include <assert.h>
#include "../../utility.h"
#include "../../amosum.h"
#include <sstream>
#include <iostream>
#include <vector>
#include <limits>

const std::vector<clingo_literal_t>* propagation_phase_ge_eo(const Group* G, AmoSumPropagator* propagator) {

    propagator->S.clear();
    // std::unordered_map<clingo_literal_t, int> sum_removed_weights;
    cmn::PerfectNHash<clingo_literal_t, int>& sum_removed_weights = *propagator->sum_removed_weights.get();

    if (propagator->sum_violated){
        
        if(!propagator->lazy_prop_activated)
            return &propagator->S ;

        clingo_literal_t l = propagator->current_literal ;
        propagator->S.push_back(not_(l));
        propagator->sum_removed_weights->set(not_(l), 0);

        auto R = get_perfect_hash_with_pointer(propagator->reason.get(), not_(l));
        R->clear();

        bool derived_true = false;
        Group* g = propagator->group->get(l);
        if(g == nullptr){
            derived_true = true ;
            g = propagator->group->get(not_(l));
        }
        
        create_reason_falses_ge(propagator, sum_removed_weights, not_(l));
    
        if(derived_true){
            clingo_literal_t sml_g = max_w(g) ;
            create_reason_true_ge(propagator, sml_g, not_(l), g, sum_removed_weights);
        }

        propagator->compute_minimal_reason(propagator->S);
        // printDerivation(propagator->atomNames, propagator->S, false);
        return &propagator->S;
    }

    for (Group* g : propagator->groups) {
        if (g == G || propagator->true_group->getTrueLiteral(g) != SETTINGS::NONE) continue;
        int ml_g = max_w(g);
        assert((propagator->true_group->getTrueLiteral(g) != SETTINGS::NONE || ml_g != SETTINGS::NONE) || propagator->dl == 0 || g->N == 0);
        if(ml_g == SETTINGS::NONE) continue ;

        for (int l : g->ord_l) {
            if (propagator->I->get(l) == SETTINGS::NONE) {
                int mpsH = std::get<0>(propagator->mpsH(l, true));
                if (mpsH < propagator->lb) {
                    
                    if(!propagator->is_true(not_(l))) {
                        propagator->S.push_back(not_(l));
                        propagator->mpsHDuringPropagation->set(not_(l), mpsH);
                        propagator->sum_removed_weights->set(not_(l), 0);
                        auto R = get_perfect_hash_with_pointer(propagator->reason.get(), not_(l));
                        R->clear();
                    }

                } else {
                    break;
                }
            }
        }

    }

    if (!propagator->S.empty() && propagator->dl != 0) {
        create_reason_falses_ge(propagator, sum_removed_weights, SETTINGS::NONE);
        propagator->compute_minimal_reason(propagator->S);
    }

    printDerivation(propagator->atomNames, propagator->S);
    
    return &propagator->S;
}


