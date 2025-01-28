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


const std::vector<clingo_literal_t>* propagation_phase_le_eo(const Group* G, AmoSumPropagator* propagator) {
    // Clear the reason vector and derived literals set

    propagator->S.clear();

    // Handle case when mps_violated is true
    if (propagator->mps_violated) {
        clingo_literal_t l = propagator->current_literal;
        propagator->S.push_back(not_(l));

        auto R = get_perfect_hash_with_pointer(propagator->reason.get(), not_(l));
        R->clear();

        bool derived_true = false;
        Group* g = propagator->group->get(l);
        if (g == nullptr) {
            derived_true = true;
            g = propagator->group->get(not_(l));
        }

        create_reason_falses_le(propagator, not_(l));

        if (derived_true) {
            clingo_literal_t sml_g = min_w(g);
            create_reason_true_le(propagator, sml_g, not_(l), g);
        }

        print_derivation(propagator->atomNames, propagator->S, false);
        return &propagator->S;
    }

    // Iterate over all groups
    for (Group* g : propagator->groups) {
        if (g == G || propagator->true_group->get(g) != SETTINGS::NONE) {
            continue;
        }

        for (int i = g->ord_l.size() - 1; i >= 0; --i) {
            clingo_literal_t l = g->ord_l[i];
            if (propagator->I->get(l) == SETTINGS::NONE) {
                if (std::get<0>(propagator->mps(g, l, true)) > propagator->ub) {
                    // Infer l as false
                    if (!propagator->to_be_propagated->get(not_(l))) {
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

    // Update reason if necessary
    if (!propagator->S.empty() && propagator->dl != 0) {
        create_reason_falses_le(propagator, SETTINGS::NONE);
    }

    print_derivation(propagator->atomNames, propagator->S, false);
    return &propagator->S;
}
