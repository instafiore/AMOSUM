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
    #ifndef PRIVATE_REASON
    propagator->reason_falses.clear();
    #endif
    propagator->S.clear();

    if (propagator->mps_violated) {
        
        clingo_literal_t l = propagator->current_literal ;
        propagator->S.push_back(not_(l));

        #ifdef PRIVATE_REASON
        auto R = get_perfect_hash_with_pointer(propagator->reason.get(), not_(l));
        R->clear();
        #endif

        bool derived_true = false;
        Group* g = propagator->group->get(l);
        if(g == nullptr){
            derived_true = true ;
            g = propagator->group->get(not_(l));
        }
        
        create_reason_falses_ge(propagator, not_(l));
        
        if(derived_true){
            clingo_literal_t sml_g = max_w(g) ;
            create_reason_true_ge(propagator, sml_g, not_(l), g);
        }

    
        print_derivation(propagator->atomNames, propagator->S, false);
        return &propagator->S;
    }

    std::vector<clingo_literal_t> derived_true ;
    for (Group* g : propagator->groups) {
        if (g == G || propagator->true_group->get(g) != SETTINGS::NONE) continue;
        int ml_g = max_w(g);
        if(ml_g == SETTINGS::NONE) continue ;

        for (int l : g->ord_l) {
            if (propagator->I->get(l) == SETTINGS::NONE) {
                if (std::get<0>(propagator->mps(g, l, true)) < propagator->lb) {
                    
                    if(!propagator->to_be_propagated->get(not_(l))) {
                        propagator->to_be_propagated->set(not_(l), true);
                        propagator->S.push_back(not_(l));
                        #ifdef PRIVATE_REASON
                        auto R = get_perfect_hash_with_pointer(propagator->reason.get(), not_(l));
                        R->clear();
                        #endif
                    }

                } else {
                    break;
                }
            }
        }

    }

    
    if (!propagator->S.empty() && propagator->dl != 0) {
        create_reason_falses_ge(propagator, SETTINGS::NONE);
        propagator->compute_minimal_reason(propagator->S);
    }

    print_derivation(propagator->atomNames, propagator->S, false);
    
    return &propagator->S;
}


