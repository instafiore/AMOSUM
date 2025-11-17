#!/home/s.fiorentino/miniconda3/bin/python3
import sys
import os
sys.path.append(os.path.dirname(os.path.abspath(__file__)))
sys.path.append(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
import amowasp
from typing import List
from amowasp.propagator_wasp_py.propagator_wasp import *
import amowasp.propagator_wasp_py.propagator_wasp as propagator_wasp
from utility import *
import time

'''
Propagator for ' >= LB  ' constraint and At Most One constraint

Invariants:
    In the aggregate set there are not two literals such that li = ~lj
'''

def propagate_phase(G: Group, propagator: AmoSumPropagator, atomNames: dict):
       
    propagator.S = []
    sum_removed_weights = {}
    
    # debug(f"Propagate Phase decision level {propagator.dl}", force_print=True)
    if propagator.mps_violated:

        assert propagator.lazy_prop_activated or propagator.dl == 0
        l = propagator.current_literal
        propagator.reason[not_(l)] = [] if propagator.solver == AmoSumPropagator.CLINGO or propagator.dl == 0 else [not_(propagator.current_literal)]
        propagator.S = [not_(l)]

        derived_true = False
        g = propagator.group[l]
        if(g is None):
            derived_true = True
            g = propagator.group[not_(l)]

        create_reason_falses_ge(propagator, sum_removed_weights, not_(l))

        if derived_true:
            sml_g = max_w(g)
            create_reason_true_ge(propagator, sml_g, not_(l), g, sum_removed_weights)


        print_derivation(propagator.atomNames, propagator.S, force_print=False)  
        propagator.to_be_propagated[not_(l)] = True
        return propagator.S
    
    derived_true = []
 
    for g in propagator.groups:
        if g == G or not propagator.true_group[g] is None:
            continue

        ml_g =  max_w(g)
 
        if ml_g is None:
            continue
        
        istrue = propagator.to_be_propagated[ml_g] if propagator.solver == AmoSumPropagator.WASP else propagator.is_true(ml_g)
        if istrue: continue 
        
        mps, sml_g, ml_g = propagator.mps(g, ml_g, assumed=False, return_literals=True)
        propagate_to_true = False
        if mps < propagator.lb:
            propagator.S.append(ml_g)
            derived_true.append(ml_g)
            propagator.reason[ml_g] = [] if propagator.solver == AmoSumPropagator.CLINGO or propagator.dl == 0 else [not_(propagator.current_literal)]
            create_reason_true_ge(propagator, sml_g, ml_g, g, sum_removed_weights)
            propagator.to_be_propagated[ml_g] = True
            
            propagate_to_true = True
        
      
        if not propagate_to_true:
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

      
    if len(propagator.S) != 0  and propagator.dl != 0:
        create_reason_falses_ge(propagator=propagator, sum_removed_weights=sum_removed_weights)
        propagator.compute_minimal_reason(to_minimize=propagator.S)

    
    print_derivation(propagator.atomNames, propagator.S, force_print=False)
    return propagator.S
