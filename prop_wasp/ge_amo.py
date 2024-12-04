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

    if propagator.mps_violated:
        propagator.reason = create_reason_falses_ge(propagator=propagator)
        return [not_(propagator.current_literal)]


    # set of derived literals
    S : List[int] = []

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
        propagator.reason = create_reason_falses_ge(propagator=propagator)
        propagator.compute_minimal_reason(reason=propagator.reason, derived=S)

    print_derivation(propagator.atomNames, S)

    return S
