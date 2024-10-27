#!/usr/bin/python3
import sys
import os
sys.path.append(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
import wasp_dir
from typing import *
from utility import *
from functional_propagator import *
import functional_propagator

'''
Propagator for ' >= LB ' constraint with Exactly One constraint 
Invariants:
    In the aggregate set there are not two literals such that li = ~lj
'''
functional_propagator.propagator.prob_type = "EO"
functional_propagator.propagator.ge = True

def propagate_phase(G: Group, propagator: PropagatorWasp):

    # set of derived literals
    S : List[int] = []
    
    # reason
    R : List[int] = []

    for g in propagator.groups:
        if g == G or not propagator.true_group[g] is None:
            continue

        ml_g =  max_w(g)
        mw_g =  propagator.weight[ml_g]

        for l in g.ord_l:
            if propagator.I[l] is None:
                if propagator.mps - mw_g + propagator.weight[l] < propagator.lb:
                    # infer l as false
                    S.append(not_(l))
                else:
                    break

    if len(S) != 0:
        for g in propagator.groups:
            if propagator.true_group[g] is None:
                ord_l = g.ord_l_origin 
                mw_g = propagator.weight[max_w(g)]
                for i in range(len(ord_l) - 1, -1, -1):
                    l = ord_l[i]
                    if propagator.weight[l] <= mw_g:
                        break
                    R.append(l)
            else:
                R.append(not_(propagator.true_group[g]))

        # updating the reason
        propagator.reason_falses = R
        propagator.compute_minimal_reason(reason=R)
    print_derivation(propagator.atomNames, S)

     
    return S

propagator.propagate_phase = propagate_phase


