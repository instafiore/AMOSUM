#!/home/s.fiorentino/miniconda3/bin/python3
import sys
import os
sys.path.append(os.path.dirname(os.path.abspath(__file__)))
sys.path.append(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
import prop_wasp
from typing import List
from prop_wasp.propagator_wasp_py.propagator_wasp import *
import prop_wasp.propagator_wasp_py.propagator_wasp as propagator_wasp
from utility import *
import time

'''
Propagator for ' >= LB  ' constraint and At Most One constraint

Invariants:
    In the aggregate set there are not two literals such that li = ~lj
'''

def propagate_phase(G: Group, propagator: AmoSumPropagator, atomNames: dict):

    propagator.reason = []    
    propagator.S = []

    if propagator.mps_violated:
        assert propagator.lazy_prop_activated

        l = propagator.current_literal

        S = [not_(l)]

        propagator.reason = []

        derived_true = False
        g = propagator.group[l]
        if(g is None):
            derived_true = True
            g = propagator.group[not_(l)]

        create_reason_falses_ge(propagator, not_(l))

        if derived_true:
            sml_g = max_w(g)
            create_reason_true_ge(propagator, sml_g, not_(l), g)

        return propagator.S
    
    derived_true = []
    # arrived here
    for g in propagator.groups:
        if g == G or not propagator.true_group[g] is None:
            continue

        ml_g =  max_w(g)
        if ml_g is None:
            continue

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

      
    if len(S) != 0  and propagator.dl != 0:
        create_reason_falses_ge(propagator=propagator)
        propagator.compute_minimal_reason(reason=propagator.reason, derived=S)

    print_derivation(propagator.atomNames, S)

    return S
