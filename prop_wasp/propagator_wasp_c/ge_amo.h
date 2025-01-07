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

    if (propagator->mps_violated) {
        create_reason_falses_ge(propagator);
        propagator->S.clear();
        propagator->S.push_back(not_(propagator->current_literal));
        print_derivation(propagator->atomNames, propagator->S, false);
        return &propagator->S;
    }

    // auto start = start_timer();
    propagator->S.clear();
    std::vector<clingo_literal_t> derived_true ;
    for (Group* g : propagator->groups) {
        if (g == G || propagator->true_group->get(g) != SETTINGS::NONE) continue;
        int ml_g = max_w(g);
        if (ml_g == SETTINGS::NONE) continue;

        auto [mps_h, sml_g, ml_g_res] = propagator->mps(g, ml_g, false);
        
        bool propagate_to_true = false;
        if (mps_h < propagator->lb) {
            int i = sml_g != SETTINGS::NONE ? g->ord_i[sml_g] : SETTINGS::NONE;
            int j = g->ord_i[ml_g_res];
            auto rst = propagator->reason_trues->get(ml_g);
            if(rst == nullptr) {
                rst = new std::vector<clingo_literal_t>();
                propagator->reason_trues->set(ml_g, rst);
            }
            else rst->clear();
            
            for (int k = i; propagator->dl != 0 and k < j; ++k) {
                clingo_literal_t lit = g->ord_l[k];
                if (!propagator->I->get(lit)) {
                    rst->push_back(lit);
                }
            }
            if(!propagator->to_be_propagated->get(ml_g_res)) {
                propagator->to_be_propagated->set(ml_g_res, true);
                propagator->S.push_back(ml_g_res);
                derived_true.push_back(ml_g_res);
            }
            // propagator->propagated[ml_g_res] = true;
            propagate_to_true = true;
            
        }

        if (!propagate_to_true) {
            for (int l : g->ord_l) {
                if (propagator->I->get(l) == SETTINGS::NONE) {
                    if (std::get<0>(propagator->mps(g, l, true)) < propagator->lb) {
                        if(!propagator->to_be_propagated->get(not_(l))) {
                            propagator->to_be_propagated->set(not_(l), true);
                            propagator->S.push_back(not_(l));
                        }
                        // propagator->propagated[not_(l)] = true;
                    } else {
                        break;
                    }
                }
            }
        }
    }

    
    if (!propagator->S.empty() && propagator->dl != 0) {
        create_reason_falses_ge(propagator);
        // auto start = start_timer() ;
        propagator->compute_minimal_reason(derived_true);
        // display_end_timer(start, "compute_minimal_reason");
    }

    print_derivation(propagator->atomNames, propagator->S, false);
    
    // display_end_timer(start, "propagation_phase whole");
    return &propagator->S;
}