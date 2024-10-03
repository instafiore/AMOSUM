#!/usr/bin/python3
import wasp
from typing import List
from utility import *
import re
from common import *
import common

'''
Propagator for ' <= UB ' constraint with At Most One constraint 

Invariants:
    In the aggregate set there are not two literals such that li = ~lj
'''

common.PROB_TYPE = "AMO"
common.GE = False

def propagate_phase(G: Group):
    global N,ub, I, weight, aggregate, groups, mps, group, reason, true_group

    # set of derived literals
    S : List[int] = []
    
    # reason
    R : List[int] = []
    
    # print_I(I=common.I, atomNames=atomNames, aggregate=aggregate)

    for g in common.groups:
        if not G is None and ( g == G or not common.true_group[g] is None ):
            continue

        ml_g =  common.min_w(g)
        mw_g =  common.weight[ml_g]

        for i in range(len(g.ord_l)-1,-1,-1):
            l = g.ord_l[i]
            if common.I[l] is None:
                if common.mps - mw_g + common.weight[l] > common.ub:
                    # infer l as false
                    S.append(not_(l))
                    g.decrease_und()
                else:
                    break

    
    if len(S) != 0:
        for g in common.groups:
            if common.true_group[g] is None:
                # debug("g", g.id, "min_w", get_name(atomNames, min_w(g)))
                mw_g = common.weight[min_w(g)]
                for l in g.ord_l:
                    if common.weight[l] >= mw_g:
                        break
                    R.append(l)
            else:
                R.append(not_(common.true_group[g]))
   
        # updating the reason
        common.reason_falses = R

    S_str = convert_array_to_string(name="Derived", array=S, atomNames=atomNames)
    debug(S_str)
    R_str = convert_array_to_string(name="Reason", array=R, atomNames=atomNames)
    debug(R_str)

    return S

common.propagate_phase = propagate_phase
