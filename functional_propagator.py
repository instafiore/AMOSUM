import sys
import os
sys.path.append(os.path.dirname(os.path.abspath(__file__)))
from ast import Tuple
from typing import Callable, List
import wasp_dir.wasp as wasp
import re
import settings
from propagator_wasp import *

atomNames = {}

# input parameters
sys_parameters = []

propagator = PropagatorWasp(atomsNames=atomNames, sys_parameters=sys_parameters)

def checkAnswerSet(*answer_set):
    global propagator
    return propagator.checkAnswerSet(*answer_set)

def getReasonsForCheckFailure():
    global propagator
    propagator.getReasonsForCheckFailure()

def getLiterals(*lits):
    global propagator
    debug(f"atoms names: {atomNames}")
    get_literals = propagator.getLiterals(*lits)
    return get_literals

def simplifyAtLevelZero():
    global propagator
    simplify_atLevel_zero = propagator.simplifyAtLevelZero(delete_lits=True)
    return simplify_atLevel_zero

def onLiteralTrue(lit, dl):
    global propagator
    name = get_name(lit=lit, atomNames=atomNames)
    debug(f"propagate [{name}, {dl}]")
    S = propagator.onLiteralTrue(lit, dl)
    return S

def getReasonForLiteral(lit):
    global propagator
    return propagator.getReasonForLiteral(lit)

def onLiteralsUndefined(*lits) -> None:
    global propagator
    undefined_lits = [get_name(lit=l, atomNames = atomNames) for l in lits]
    debug(f"undo {undefined_lits}")
    return propagator.onLiteralsUndefined(*lits)
