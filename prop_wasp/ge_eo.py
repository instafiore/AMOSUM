#!/usr/bin/python3
import sys
import os
sys.path.append(os.path.dirname(os.path.abspath(__file__)))
sys.path.append(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
import prop_wasp
from typing import *
from utility import *
from prop_wasp.propagator_wasp import *
import prop_wasp.propagator_wasp as propagator_wasp

'''
Propagator for ' >= LB ' constraint with Exactly One constraint 
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
        
        for l in g.ord_l:
            if propagator.I[l] is None:
                if propagator.mps(g, l, assumed=True) < propagator.lb:
                    S.append(not_(l))
                    propagator.propagated[not_(l)] = True
                else:
                    break

    propagator.reason = [] 
    if len(S) != 0 and propagator.dl != 0:
        # updating the reason
        propagator.reason = propagator.reason = create_reason_falses_ge(propagator=propagator)
        propagator.compute_minimal_reason(reason=propagator.reason, derived=S)
        
    print_derivation(propagator.atomNames, S)

     
    return S


