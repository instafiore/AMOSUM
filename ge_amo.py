#!/home/s.fiorentino/miniconda3/bin/python3
import wasp
from typing import List
from utility import *
from common import *
import common

'''
Propagator for ' >= LB  ' constraint and At Most One constraint

Invariants:
    In the aggregate set there are not two literals such that li = ~lj
'''

common.PROB_TYPE = "GE_AMO"
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

        count_infered_falses = 0 
        false_lits_g = []
        # infer falsity
        for l in g.ord_l:
            if common.I[l] is None:
                if common.mps - mw_g + common.weight[l] < common.lb:
                    S.append(not_(l))
                    count_infered_falses += 1
                else:
                    break
            elif common.I[l] is False:
                false_lits_g.append(l)

        if g.count_undef - count_infered_falses == 1 and common.true_group[g] is None:
            # the last remained literal 
            l = max_w(g)
            if common.mps - common.weight[l] < common.lb:
                S.append(l)
                common.reason_trues[l] = false_lits_g
    if len(S) != 0:
        for g in common.groups:
            if g.count_undef == 0 and common.true_group[g] is None:
                R.extend(reversed(g.ord_l))
            elif common.true_group[g] is None:
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

    S_str = convert_array_to_string(name="Derived", array=S, atomNames=atomNames)
    debug(S_str)
    R_str = convert_array_to_string(name="Reason", array=R, atomNames=atomNames)
    debug(R_str)
    
    # print_I(I=I, atomNames=atomNames, aggregate=aggregate)
    compute_minimal_reason(reason=R)
     
    return S

common.propagate_phase = propagate_phase
