#!/home/s.fiorentino/miniconda3/bin/python3

import sys
import os
sys.path.append(os.path.dirname(os.path.abspath(__file__)))
sys.path.append(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
import prop_wasp
from typing import List, Set
from prop_wasp.propagator_wasp import *
import prop_wasp.propagator_wasp as propagator_wasp
from utility import *

'''
Propagator for ' >= LB  ' constraint and At Most One constraint

Invariants:
    In the aggregate set there are not two literals such that li = ~lj
'''

propagator_wasp.propagator.prob_type = "AMO"
propagator_wasp.propagator.ge = True


def propagate_phase(G: Group, propagator: Propagator, atomNames: dict):
    
    # try:

    # set of derived literals
    S : List[int] = []
    
    # reason
    R : List[int] = []

    checked_lits = set()

    for g in propagator.groups:
        if g == G or not propagator.true_group[g] is None:
            continue
        
        ml_g =  max_w(g)
        if ml_g is None:
            continue

        mw_g =  propagator.weight[(ml_g, g)]

        if ml_g in checked_lits:
            continue
        
        checked_lits.add(ml_g)

        mps, involved_lits = propagator.mps(ml_g, assumed=False, return_involved_lits=True)
        propagate_to_true = False
        if mps < propagator.lb:
            for sml_g, ml_g, g in involved_lits:
                j = g.ord_i[ml_g] 
                i = g.ord_i[sml_g] if not sml_g is None else 0
                propagator.reason_trues[ml_g] = [lit for lit in g.ord_l[i:j] if propagator.I[lit] == False]
            S.append(ml_g)
            propagate_to_true = True
        
        if not propagate_to_true:
            start = g.max_und if not g.max_und is None else 0
            for i in range(start,-1,-1):
                l = g.ord_l[i]
                if not_(l) in checked_lits:
                    continue
                if propagator.I[l] is None:
                    mps_assumed_true_l = propagator.mps(l, assumed=True)
                    if mps_assumed_true_l < propagator.lb:
                        # derived_to_false = [not_(lit) for lit in g.ord_l[i::-1] if propagator.I[lit] is None]
                        # checked_lits = checked_lits.union(set(derived_to_false))
                        # NOTE: you cannot do this because a lower literal in weight 
                        # can lead to a greater mps
                        # S.extend(derived_to_false)
                        # break
                        S.append(not_(l))

    propagator.reason = []      
    if len(S) != 0  and propagator.dl != 0:
        for g in propagator.groups:
            ord_l = g.ord_l
            if g.count_undef == 0 and propagator.true_group[g] is None:
                R.extend(reversed(ord_l))
            elif propagator.true_group[g] is None:
                mw_g = propagator.weight[(max_w(g), g)]
                for i in range(len(ord_l) - 1, -1, -1):
                    l = ord_l[i]
                    if propagator.weight[(l, g)] < mw_g:
                        break
                    R.append(l) if not propagator.I[l] is None else None
            elif not g.true_from_facts :
                R.append(not_(propagator.true_group[g]))
    
        propagator.reason = R
        propagator.compute_minimal_reason(reason=R, derived=S)

    print_derivation(propagator.atomNames, S)

    return S

propagator.propagate_phase = propagate_phase
