import sys
import os
sys.path.append(os.path.dirname(os.path.abspath(__file__)))
from ast import Tuple
from typing import Callable, List
import wasp._wasp as _wasp
import re
import settings
from amosum import *

atomNames = {}

# input parameters
sys_parameters = []
propagator = Propagator(atomsNames=atomNames, sys_parameters=sys_parameters, solver=Propagator.WASP)

def checkAnswerSet(*answer_set):
    global propagator
    return propagator.checkAnswerSet(*answer_set)

def getReasonsForCheckFailure():
    global propagator
    propagator.getReasonsForCheckFailure()

def getLiterals(*lits):
    global propagator
    get_literals = propagator.getLiterals(*lits)
    debug(f"atoms names: {atomNames}")
    return get_literals

def simplifyAtLevelZero():
    global propagator
    simplify_atLevel_zero = propagator.simplifyAtLevelZero(delete_lits=True)
    return simplify_atLevel_zero

def onLiteralTrue(lit, dl):
    global propagator
    propagator.updated_dl(lit, dl)
    print_propagate(propagator=propagator, changes=[lit], dl=dl, wasp_b=True)
    S = propagator.onLiteralTrue(lit, dl)
    return S

def getReasonForLiteral(lit):
    global propagator
    return propagator.getReasonForLiteral(lit)

def onLiteralsUndefined(*lits) -> None:
    global propagator
    print_undo(propagator, lits, 0, wasp_b=True)
    return propagator.onLiteralsUndefined(*lits)
