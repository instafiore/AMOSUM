#!/usr/bin/python3
import wasp
from typing import *
from utility import *
from common import *
import common

'''
Propagator for ' >= LB ' constraint with Exactly One constraint 
Invariants:
    In the aggregate set there are not two literals such that li = ~lj
'''
common.PROB_TYPE = "GE_EO"
common.GE = True

def propagate_phase(G: Group):

    # set of derived literals
    S : List[int] = []
    
    # reason
    R : List[int] = []

    for g in common.groups:
        if g == G or not common.true_group[g] is None:
            continue

        ml_g =  max_w(g)
        mw_g =  common.weight[ml_g]

        for l in g.ord_l:
            if common.I[l] is None:
                if common.mps - mw_g + common.weight[l] < common.lb:
                    # infer l as false
                    S.append(not_(l))
                else:
                    break

    if len(S) != 0:
        for g in common.groups:
            if common.true_group[g] is None:
                mw_g = common.weight[max_w(g)]
                for i in range(len(g.ord_l) - 1, -1, -1):
                    l = g.ord_l[i]
                    if common.weight[l] <= mw_g:
                        break
                    R.append(l)
            else:
                R.append(not_(common.true_group[g]))

        # updating the reason
        common.reason_falses = R

    # S_str = convert_array_to_string(name="Derived", array=S, atomNames=atomNames)
    # debug(S_str)
    # R_str = convert_array_to_string(name="Reason", array=R, atomNames=atomNames)
    # debug(R_str)

    compute_minimal_reason(reason=R)
     
    return S

common.propagate_phase = propagate_phase



