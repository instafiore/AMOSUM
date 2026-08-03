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


const std::vector<clingo_literal_t>* propagation_phase_le_amo(const Group* G, AmoSumPropagator* propagator) {
    // Clear the reason vector and derived literals set

    propagator->S.clear();

    // Handle case when mps_violated is true
    if (propagator->sum_violated) {

        clingo_literal_t l = propagator->current_literal;
        propagator->S.push_back(not_(l));

        auto R = get_perfect_hash_with_pointer(propagator->reason.get(), not_(l));
        R->clear();

        Group* g = propagator->group->get(l);
        if (g == nullptr) {
            g = propagator->group->get(not_(l));
        }

        create_reason_le_amo(propagator, not_(l));

        printDerivation(propagator->atomNames, propagator->S);
        return &propagator->S;
    }

    // Iterate over all groups
    for (Group* g : propagator->groups) {
        if (g == G || propagator->true_group->getTrueLiteral(g) != SETTINGS::NONE) {
            continue;
        }

        for (int i = g->ord_l.size() - 1; i >= 0; --i) {
            clingo_literal_t l = g->ord_l[i];
            // We need to add !propagator->is_true(not_(l))) because we do not know the real truth value of l given that we may do not watch it
            if (propagator->I->get(l) == SETTINGS::NONE && !propagator->is_true(not_(l))) {
                if (std::get<0>(propagator->mpsH(l, true)) > propagator->ub) {
                    // Infer l as false
                    propagator->S.push_back(not_(l));
                    auto R = get_perfect_hash_with_pointer(propagator->reason.get(), not_(l));
                    R->clear();
                } else {
                    break;
                }
            }
        }
    }

    // Update reason if necessary
    if (!propagator->S.empty() && propagator->dl != 0) {
        create_reason_le_amo(propagator, SETTINGS::NONE);
    }

    printDerivation(propagator->atomNames, propagator->S);
    return &propagator->S;
}
