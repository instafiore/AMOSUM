#!/home/s.fiorentino/miniconda3/bin/python3
import sys
import os
sys.path.append(os.path.dirname(os.path.abspath(__file__)))
sys.path.append(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
import prop_wasp
from typing import List
from prop_wasp.propagator_wasp import *
import prop_wasp.propagator_wasp as propagator_wasp
from utility import *

'''
Propagator for ' >= LB  ' constraint and At Most One constraint

Invariants:
    In the aggregate set there are not two literals such that li = ~lj
'''

def propagate_phase(G: Group, propagator: AmoSumPropagator, atomNames: dict):
    
    # try:

    # set of derived literals
    S : List[int] = []
    
    # reason
    R : List[int] = []

    for g in propagator.groups:
        if g == G or not propagator.true_group[g] is None:
            continue

        ml_g =  max_w(g)
        if ml_g is None:
            continue

        mw_g =  propagator.weight[ml_g]

        mps, sml_g, ml_g = propagator.mps(g, ml_g, assumed=False, return_literals=True)
        propagate_to_true = False
        if mps < propagator.lb:
            i = g.ord_i[sml_g] if not sml_g is None else 0
            j = g.ord_i[ml_g]
            propagator.reason_trues[ml_g] = [lit for lit in g.ord_l[i:j] if propagator.I[lit] == False]
            S.append(ml_g)
            propagator.propagated[ml_g] = True
            propagate_to_true = True
        
        if not propagate_to_true:
            for l in g.ord_l:
                if propagator.I[l] is None:
                    if propagator.mps(g, l, assumed=True) < propagator.lb:
                        S.append(not_(l))
                        propagator.propagated[not_(l)] = True
                    else:
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
                    if propagator.weight[l] < mw_g:
                        break
                    R.append(l) if not propagator.I[l] is None else None
            else:
                R.append(not_(propagator.true_group[g]))
    
        propagator.reason = R
        propagator.compute_minimal_reason(reason=R, derived=S)

    print_derivation(propagator.atomNames, S)

    return S
