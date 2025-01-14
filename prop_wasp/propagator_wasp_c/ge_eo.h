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
    // Handle the case where the MPS condition is violated
    if (propagator->mps_violated) {
        propagator->reason.clear();
        create_reason_falses_ge(propagator); // Update the reason for falses
        propagator->S.clear();
        propagator->S.push_back(not_(propagator->current_literal));
        print_derivation(propagator->atomNames, propagator->S, false);
        return &propagator->S;
    }

    // Clear the set of derived literals
    propagator->S.clear();

    // Iterate over all groups
    for (Group* g : propagator->groups) {
        // Skip the current group or any group already determined to be true
        if (g == G || propagator->true_group->get(g) != SETTINGS::NONE) {
            continue;
        }

        // Process the literals in the group
        for (int l : g->ord_l) {
            // Check if the literal is undefined in the interpretation
            if (propagator->I->get(l) == SETTINGS::NONE) {
                // Check if the MPS condition holds
                auto mps_result = propagator->mps(g, l, true);
                if (std::get<0>(mps_result) < propagator->lb) {
                    propagator->S.push_back(not_(l));
                    propagator->to_be_propagated->set(not_(l), true);
                } else {
                    break;
                }
            }
        }
    }

    // Clear the reason and update it if necessary
    propagator->reason.clear();
    if (!propagator->S.empty() && propagator->dl != 0) {
        create_reason_falses_ge(propagator);
        propagator->compute_minimal_reason(propagator->S);
    }

    // Print the derivation and return the set of derived literals
    print_derivation(propagator->atomNames, propagator->S, false);
    return &propagator->S;
}


