#!/home/s.fiorentino/miniconda3/bin/python3
import sys
import os
sys.path.append(os.path.dirname(os.path.abspath(__file__)))
sys.path.append(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
import wasp
from typing import List
from functional_propagator import *
import functional_propagator
from utility import *

'''
Propagator for ' >= LB  ' constraint and At Most One constraint

Invariants:
    In the aggregate set there are not two literals such that li = ~lj
'''

functional_propagator.propagator.prob_type = "AMO"
functional_propagator.propagator.ge = True


def propagate_phase(G: Group, propagator: PropagatorWasp, atomNames: dict):
    
    # try:

    # set of derived literals
    S : List[int] = []
    
    # reason
    R : List[int] = []

    for g in propagator.groups:
        if g == G or not propagator.true_group[g] is None:
            continue

        ml_g =  max_w(g)
        mw_g =  propagator.weight[ml_g]

        for i in range(len(g.ord_l)-1,-1,-1):
            l = g.ord_l[i]
            if propagator.I[l] is None:
                if propagator.mps(g, l, assumed=True) < propagator.lb:
                    S.extend([not_(lit) for lit in g.ord_l[i::-1] if propagator.I[lit] is None])
                    break
                else:
                    mps, sml_g, ml_g = propagator.mps(g, l, assumed=False, return_literals=True)
                    if mps < propagator.lb:
                        i = g.ord_i[sml_g] if not sml_g is None else 0
                        propagator.reason_trues[l] = [lit for lit in g.ord_l[i::] if propagator.I[lit] == False]
                        S.append(l)
                        break

    propagator.reason = []      
    if len(S) != 0  and propagator.dl != 0:
        for g in propagator.groups:
            ord_l = g.ord_l
            if g.count_undef == 0 and propagator.true_group[g] is None:
                R.extend(reversed(ord_l))
            elif propagator.true_group[g] is None:
                mw_g = propagator.weight[max_w(g)]
                for i in range(len(ord_l) - 1, -1, -1):
                    l = ord_l[i]
                    if propagator.weight[l] <= mw_g:
                        break
                    R.append(l) 
            else:
                R.append(not_(propagator.true_group[g]))
    
        propagator.reason = R
        propagator.compute_minimal_reason(reason=R, derived=S)

    print_derivation(propagator.atomNames, S)

    return S

propagator.propagate_phase = propagate_phase
