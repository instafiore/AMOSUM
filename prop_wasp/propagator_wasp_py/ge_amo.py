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
       
    propagator.S = []
    
    if propagator.mps_violated:

        assert propagator.lazy_prop_activated
        l = propagator.current_literal
        propagator.reason[not_(l)] = [] if propagator.solver == AmoSumPropagator.CLINGO else [not_(propagator.current_literal)]
        propagator.S = [not_(l)]

        derived_true = False
        g = propagator.group[l]
        if(g is None):
            derived_true = True
            g = propagator.group[not_(l)]

        create_reason_falses_ge(propagator, not_(l))

        if derived_true:
            sml_g = max_w(g)
            create_reason_true_ge(propagator, sml_g, not_(l), g)


        print_derivation(propagator.atomNames, propagator.S, force_print=False)  
        propagator.propagated[not_(l)] = True
        return propagator.S
    
    derived_true = []
 
    for g in propagator.groups:
        if g == G or not propagator.true_group[g] is None:
            continue

        ml_g =  max_w(g)
 
        if ml_g is None:
            continue

        if propagator.to_be_propagated[ml_g]: continue 
        
        mps, sml_g, ml_g = propagator.mps(g, ml_g, assumed=False, return_literals=True)
        propagate_to_true = False
        if mps < propagator.lb:
            if not propagator.to_be_propagated[ml_g]:
                propagator.to_be_propagated[ml_g] = True
                propagator.S.append(ml_g)
                derived_true.append(ml_g)
                propagator.reason[ml_g] = [] if propagator.solver == AmoSumPropagator.CLINGO else [not_(propagator.current_literal)]
                create_reason_true_ge(propagator, sml_g, ml_g, g)
                propagator.propagated[ml_g] = True
            
            propagate_to_true = True
        
      
        if not propagate_to_true:
            for l in g.ord_l:
                if propagator.I[l] is None:
                    if propagator.mps(g, l, assumed=True) < propagator.lb:
                        if not propagator.to_be_propagated[not_(l)]:
                            propagator.to_be_propagated[not_(l)] = True
                            propagator.S.append(not_(l))
                            propagator.reason[not_(l)] = [] if propagator.solver == AmoSumPropagator.CLINGO else [not_(propagator.current_literal)]
                            propagator.propagated[not_(l)] = True
                    else:
                        break

      
    if len(propagator.S) != 0  and propagator.dl != 0:
        create_reason_falses_ge(propagator=propagator)
        propagator.compute_minimal_reason(to_minimize=propagator.S)

    
    print_derivation(propagator.atomNames, propagator.S, force_print=False)
    return propagator.S
