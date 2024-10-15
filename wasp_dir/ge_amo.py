#!/home/s.fiorentino/miniconda3/bin/python3
import sys
import os
sys.path.append(os.path.dirname(os.path.abspath(__file__)))
sys.path.append(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
import wasp
from typing import List
from utility import *
from functional_propagator import *
import functional_propagator

'''
Propagator for ' >= LB  ' constraint and At Most One constraint

Invariants:
    In the aggregate set there are not two literals such that li = ~lj
'''

functional_propagator.propagator.prob_type = "AMO"
functional_propagator.propagator.ge = True


def propagate_phase(G: Group, propagator: PropagatorWasp, atomNames: dict):

    # set of derived literals
    S : List[int] = []
    
    # reason
    R : List[int] = []

    for g in propagator.groups:
        if g == G or not propagator.true_group[g] is None:
            continue

        ml_g =  max_w(g)
        mw_g =  propagator.weight[ml_g]

        count_infered_falses = 0 
        false_lits_g = []
        # infer falsity
        for l in g.ord_l:
            if propagator.I[l] is None:
                if propagator.mps - mw_g + propagator.weight[l] < propagator.lb:
                    S.append(not_(l))
                    count_infered_falses += 1
                else:
                    break
            elif propagator.I[l] is False:
                false_lits_g.append(l)

        if g.count_undef - count_infered_falses == 1 and propagator.true_group[g] is None:
            # the last remained literal 
            l = max_w(g)
            if propagator.mps - propagator.weight[l] < propagator.lb:
                S.append(l)
                propagator.reason_trues[l] = false_lits_g
    if len(S) != 0:
        for g in propagator.groups:
            if g.count_undef == 0 and propagator.true_group[g] is None:
                R.extend(reversed(g.ord_l))
            elif propagator.true_group[g] is None:
                mw_g = propagator.weight[max_w(g)]
                for i in range(len(g.ord_l) - 1, -1, -1):
                    l = g.ord_l[i]
                    if propagator.weight[l] <= mw_g:
                        break
                    R.append(l)
            else:
                R.append(not_(propagator.true_group[g]))

        # updating the reason
        propagator.reason_falses = R

    # S_str = convert_array_to_string(name="Derived", array=S, atomNames=atomNames)
    # debug(S_str, file=sys.stderr, force_print=False)
    # R_str = convert_array_to_string(name="Reason", array=R, atomNames=atomNames)
    # debug(R_str, file=sys.stderr, force_print=False)
    
    # print_I(I=I, atomNames=atomNames, aggregate=aggregate)
    propagator.compute_minimal_reason(reason=R)
     
    return S

propagator.propagate_phase = propagate_phase
