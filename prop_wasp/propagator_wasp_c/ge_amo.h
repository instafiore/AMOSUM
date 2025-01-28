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


const std::vector<clingo_literal_t>* propagation_phase_ge_amo(const Group* G, AmoSumPropagator* propagator){

    
    propagator->S.clear();

    if (propagator->mps_violated) {
        clingo_literal_t l = propagator->current_literal ;
        // debugf("literal: ", get_name(propagator->atomNames, l));
        assert(propagator->lazy_prop_activated);

        
        propagator->S.push_back(not_(l));

        auto R = get_perfect_hash_with_pointer(propagator->reason.get(), not_(l));
        R->clear();
 
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

        
        propagator->compute_minimal_reason(propagator->S);
        print_derivation(propagator->atomNames, propagator->S, false);
        return &propagator->S;
    }

    std::vector<clingo_literal_t> derived_true ;
    for (Group* g : propagator->groups) {
        if (g == G || propagator->true_group->get(g) != SETTINGS::NONE) continue;
        int ml_g = max_w(g);
        if (ml_g == SETTINGS::NONE) continue;

        if(propagator->to_be_propagated->get(ml_g)) continue ;

        auto [mps_h, sml_g, ml_g_res] = propagator->mps(g, ml_g, false);
        
        bool propagate_to_true = false;
        if (mps_h < propagator->lb) {
            
            if(!propagator->to_be_propagated->get(ml_g_res)) {
                propagator->to_be_propagated->set(ml_g_res, true);
                propagator->S.push_back(ml_g_res);
                derived_true.push_back(ml_g_res);
                auto R = get_perfect_hash_with_pointer(propagator->reason.get(), ml_g_res);
                R->clear();
                create_reason_true_ge(propagator, sml_g, ml_g, g);
            }

            propagate_to_true = true;
            
        }

        if (!propagate_to_true) {
            for (int l : g->ord_l) {
                if (propagator->I->get(l) == SETTINGS::NONE) {
                    if (std::get<0>(propagator->mps(g, l, true)) < propagator->lb) {
                        
                        if(!propagator->to_be_propagated->get(not_(l))) {
                            propagator->to_be_propagated->set(not_(l), true);
                            propagator->S.push_back(not_(l));
                            auto R = get_perfect_hash_with_pointer(propagator->reason.get(), not_(l));
                            R->clear();
                        }

                    } else {
                        break;
                    }
                }
            }
        }
    }

    print_derivation(propagator->atomNames, propagator->S, false);
    
    if (!propagator->S.empty() && propagator->dl != 0) {
        create_reason_falses_ge(propagator, SETTINGS::NONE);
        
        propagator->compute_minimal_reason(propagator->S);
        // propagator->compute_minimal_reason(derived_true);
    }

    
    return &propagator->S;
}
