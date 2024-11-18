#!/usr/bin/python3
import sys
import os
sys.path.append(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
import wasp
from typing import List
from utility import *
import re
from functional_propagator import *
import functional_propagator

'''
Propagator for ' <= UB ' constraint with Exactly One constraint 

Invariants:
    In the aggregate set there are not two literals such that li = ~lj
'''

functional_propagator.propagator.prob_type = "EO"
functional_propagator.propagator.ge = False

def propagate_phase(G: Group, propagator: PropagatorWasp):
    global N,ub, I, weight, aggregate, groups, mps, group, reason, true_group

    # set of derived literals
    S : List[int] = []
    
    # reason
    R : List[int] = []
    
    # print_I(I=common.I, atomNames=atomNames, aggregate=aggregate)

    for g in propagator.groups:
        if g == G or not propagator.true_group[g] is None:
            continue

        ml_g =  propagator.min_w(g)
        mw_g =  propagator.weight[ml_g]

        for i in range(len(g.ord_l)-1,-1,-1):
            l = g.ord_l[i]
            if propagator.I[l] is None:
                if propagator.mps(g=g, l=l, assumed=True) > propagator.ub:
                    # infer l as false
                    S.append(not_(l))
                else:
                    break

    propagator.reason = [] 
    if len(S) != 0 and propagator.dl != 0:
        for g in propagator.groups:
            if propagator.true_group[g] is None:
                mw_g = propagator.weight[min_w(g)]
                ord_l = g.ord_l
                for l in ord_l:
                    if propagator.weight[l] >= mw_g:
                        break
                    R.append(l)
            else:
                R.append(not_(propagator.true_group[g]))
   
        # updating the reason
        propagator.reason = R
    print_derivation(propagator.atomNames, S)

    return S

propagator.propagate_phase = propagate_phase
