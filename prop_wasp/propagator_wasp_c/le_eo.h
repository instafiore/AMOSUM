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


const std::vector<clingo_literal_t>* propagation_phase_le_eo(const Group &G, AmoSumPropagator &propagator){

    std::vector<clingo_literal_t>* S ;
    // S = new std::vector<clingo_literal_t>();

    // if (propagator.mps_violated) {
    //     propagator.reason=create_reason_falses_le(propagator);
    //     S->emplace_back(not_(propagator.mps_violated));
    //     return S;
    // }
    
    // // Iterate over groups and apply logic
    // for (auto& g : propagator.groups) {
    //     TrueGroupFunction true_group = *propagator.true_group;
    //     if (g == G or not true_group[g]) {
    //         continue;
    //     }

    //     int ml_g = propagator.mps(g, 0, false, true);
    //     if (ml_g == 0) {
    //         continue;
    //     }

    //     int mps, sml_g;
    //     std::tie(mps, sml_g, ml_g) = propagator.mps(g, ml_g, false, true);
    //     bool propagate_to_true = false;

    //     if (mps < propagator.lb) {
    //         int i = sml_g != 0 ? g->ord_i[sml_g] : 0;
    //         int j = g->ord_i[ml_g];
    //         for (int lit : g->ord_l) {
    //             if (propagator.I[lit] == false) {
    //                 propagator.reason_trues[ml_g].push_back(lit);
    //             }
    //         }
    //         S.push_back(ml_g);
    //         propagator.propagated[ml_g] = true;
    //         propagate_to_true = true;
    //     }

    //     if (!propagate_to_true) {
    //         for (int l : g->ord_l) {
    //             if (propagator.I.find(l) == propagator.I.end()) {
    //                 if (propagator.mps(g, l, true, false) < propagator.lb) {
    //                     S.push_back(not_(l));
    //                     propagator.propagated[not_(l)] = true;
    //                 } else {
    //                     break;
    //                 }
    //             }
    //         }
    //     }
    // }

    // // Compute minimal reason if necessary
    // propagator.reason.clear();
    // if (!S.empty() && propagator.dl != 0) {
    //     propagator.reason = propagator.create_reason_falses_ge();
    //     propagator.compute_minimal_reason(propagator.reason, S);
    // }

    // print_derivation(atomNames, S);
    return S;
}