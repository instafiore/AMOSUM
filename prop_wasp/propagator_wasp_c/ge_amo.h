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
        return &propagator->S;
    }

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
            
            for (int k = i; k < j; ++k) {
                clingo_literal_t lit = g->ord_l[k];
                if (!propagator->I->get(lit)) {
                    rst->push_back(lit);
                }
            }
            
            propagator->S.push_back(ml_g_res);
            // propagator->propagated[ml_g_res] = true;
            propagate_to_true = true;
            
        }

        // if (!propagate_to_true) {
        //     for (int l : g->ord_l) {
        //         if (propagator->I[l] == 0) {
        //             if (std::get<0>(propagator->mps(g, l, true, false)) < propagator->lb) {
        //                 S.push_back(not_(l));
        //                 propagator->propagated[not_(l)] = true;
        //             } else {
        //                 break;
        //             }
        //         }
        //     }
        // }
    }

    // propagator->reason.clear();
    // if (!S.empty() && propagator->dl != 0) {
    //     propagator->reason = create_reason_falses_ge(propagator);
    //     propagator->compute_minimal_reason(propagator->reason, S);
    // }

    // print_derivation(atomNames, S);
    return &propagator->S;
}