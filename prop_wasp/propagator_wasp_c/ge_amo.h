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

    propagator->reason_falses.clear();
    propagator->S.clear();

    if (propagator->mps_violated) {
        clingo_literal_t l = propagator->current_literal ;
        
        bool derived_true = false;
        Group* g = propagator->group->get(l);
        if(g == nullptr){
            derived_true = true ;
            g = propagator->group->get(not_(l));
        }


        int wl = propagator->weight->get(l);
        float ap = propagator->lb / static_cast<float>(propagator->_mps);
        
        assert(false);

        create_reason_falses_ge(propagator, not_(l));
        
        if(derived_true){
            auto [sml_g, ml_g] = g->update_max(propagator->I, true, false, not_(l));
            create_reason_true_ge(propagator, sml_g, not_(l), g);
        }
        // else  propagator->add_redundant_lits(not_(l), g->ord_l);

        propagator->S.push_back(not_(l));
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
            create_reason_true_ge(propagator, sml_g, ml_g, g);
            
            // if(!propagator->to_be_propagated->get(ml_g_res)) {
            //     propagator->to_be_propagated->set(ml_g_res, true);
            //     propagator->S.push_back(ml_g_res);
            //     derived_true.push_back(ml_g_res);
            // }
            propagator->S.push_back(ml_g_res);
            propagate_to_true = true;
            
        }

        if (!propagate_to_true) {
            for (int l : g->ord_l) {
                if (propagator->I->get(l) == SETTINGS::NONE) {
                    if (std::get<0>(propagator->mps(g, l, true)) < propagator->lb) {
                        
                        // if(!propagator->to_be_propagated->get(not_(l))) {
                        //     // propagator->add_redundant_lits(not_(l), g->ord_l);
                        //     propagator->to_be_propagated->set(not_(l), true);
                        //     propagator->S.push_back(not_(l));
                        // }
                        propagator->S.push_back(not_(l));

                    } else {
                        break;
                    }
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
