#!/usr/bin/python3
import sys
import os
sys.path.append(os.path.dirname(os.path.abspath(__file__)))
sys.path.append(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
import prop_wasp
from typing import *
from utility import *
from prop_wasp.propagator_wasp_py.propagator_wasp import *
import prop_wasp.propagator_wasp_py.propagator_wasp as propagator_wasp

'''
Propagator for ' >= LB ' constraint with Exactly One constraint 
Invariants:
    In the aggregate set there are not two literals such that li = ~lj
'''

def propagate_phase(G: Group, propagator: AmoSumPropagator, atomNames: dict):
    propagator.S = []
    
    if propagator.mps_violated:
        
        if not propagator.lazy_prop_activated:
            return propagator.S
        l = propagator.current_literal

        
        propagator.S = [not_(l)]
        propagator.reason[not_(l)] = [] if propagator.solver == AmoSumPropagator.CLINGO or propagator.dl == 0 else [not_(propagator.current_literal)]

        derived_true = False
        g = propagator.group[l]
        if g is None:
            derived_true = True
            g = propagator.group[not_(l)]

        create_reason_falses_ge(propagator, not_(l))

        if derived_true:
            sml_g = max_w(g)
            create_reason_true_ge(propagator, sml_g, not_(l), g)
        propagator.to_be_propagated[not_(l)] = True
        return propagator.S


    derived_true = []

    for g in propagator.groups:
        if g == G or propagator.true_group[g] is not None:
            continue

        ml_g = max_w(g)
        if ml_g is None:
            continue

        for l in g.ord_l:
            if propagator.I[l] is None:
                if propagator.mps(g, l, assumed=True) < propagator.lb:
                    istrue = propagator.to_be_propagated[not_(l)] if propagator.solver == AmoSumPropagator.WASP else propagator.is_true(not_(l))
                    if not istrue:
                        propagator.S.append(not_(l))
                        propagator.reason[not_(l)] = [] if propagator.solver == AmoSumPropagator.CLINGO or propagator.dl == 0 else [not_(propagator.current_literal)]
                        propagator.to_be_propagated[not_(l)] = True
                else:
                    break

    if propagator.S and propagator.dl != 0:
        create_reason_falses_ge(propagator=propagator)
        propagator.compute_minimal_reason(to_minimize=propagator.S)

    print_derivation(propagator.atomNames, propagator.S)
    return propagator.S



