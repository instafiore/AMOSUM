#!/home/s.fiorentino/miniconda3/bin/python3
import sys
import os
sys.path.append(os.path.dirname(os.path.abspath(__file__)))
sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..')))
from ast import Tuple
from typing import Callable, List
import re
import settings
from amosum import *

# mapping literal -> id
atomNames = {}

# input parameters
sys_parameters = []

propagators: List[AmoSumPropagator] = []

def checkAnswerSet(*answer_set):
    global propagators
    for propagator in propagators:
        propagator.checkAnswerSet(*answer_set)
    return wasp.coherent()

def getReasonsForCheckFailure():
    return None

def getLiterals(*lits):
    global propagators
    params = process_sys_parameters(sys_parameters)
    # debug(f"params: {params}", force_print=True)
    global_literals = []
    for prop_type, param in params:
        ge, propagate_phase = get_propagator_variables(prop_type=prop_type)
        propagator = AmoSumPropagator.create_propagator(atomsNames=atomNames, sys_parameters=param, propagation_phase=propagate_phase, ge=ge, prop_type=prop_type)
        get_literals = propagator.getLiterals(*lits)
        global_literals.extend(get_literals)
        propagators.append(propagator)
    return global_literals

def simplifyAtLevelZero():
    global propagators
    global_simplify_atLevel_zero = []
    for propagator in propagators:
        simplify_atLevel_zero = propagator.simplifyAtLevelZero(delete_lits=True)
        if len(propagators) > 1:
            global_simplify_atLevel_zero.extend(simplify_atLevel_zero) 
        else:
            global_simplify_atLevel_zero = simplify_atLevel_zero
    return  global_simplify_atLevel_zero

def onLiteralTrue(lit, dl):
    global propagators
    propagators[0].updated_dl(lit, dl)
    print_propagate(propagator=propagators[0], changes=[lit], dl=dl, wasp_b=True)

    global_S = []
    for propagator in propagators:
        S = propagator.onLiteralTrue(lit, dl)
        if len(propagators) > 1:
            global_S.extend(S)
        else:
            global_S = S
    return global_S

def getReasonForLiteral(lit):
    global propagators
    reason = None
    for propagator in propagators:
        if propagator.is_in_aggregate(lit):
            if propagator.propagated[lit]:
                reason = propagator.getReasonForLiteral(lit)
                break
    return reason

def onLiteralsUndefined(*lits) -> None:
    global propagators
    print_undo(propagator, lits, 0, wasp_b=True)
    for propagator in propagators:
        propagator.onLiteralsUndefined(*lits)
