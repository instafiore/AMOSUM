#!/home/s.fiorentino/miniconda3/bin/python3
import sys
import os
from typing import Sequence
# adding the root path
sys.path.append(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
import clingo
from propagator_wasp import *
import propagator_wasp
from clingo.symbol import Number, Function
from clingo.control import Control
from clingo.symbol import Function
from clingo.propagator import Propagator

class PropagatorClingo(clingo.Propagator):

    def __init__(self, sys_parameters: dict[str: str], propagation_phase: Callable[[Group, PropagatorWasp], List[int]], ge: bool, prop_type: str) -> None:
        super().__init__()
        self.sys_parameters = sys_parameters
        self.propagation_phase = propagation_phase
        self.ge = ge
        self.prop_type = prop_type

    def init(self, init: clingo.PropagateInit) -> None:
        atoms = list(init.symbolic_atoms)
        atoms_list_for_mapping = [(str(a.symbol), a.literal, init.solver_literal(a.literal)) for a in init.symbolic_atoms]
        max_plit = 0
        for str_symbol, program_literal, solver_literal in atoms_list_for_mapping:
            if max_plit < program_literal:
                max_plit = program_literal

        self.atomNames = { str_symbol : program_literal for str_symbol, program_literal, solver_literal in atoms_list_for_mapping}

        self.prop = propagator_wasp.PropagatorWasp(atomsNames=self.atomNames, sys_parameters=self.sys_parameters,
                                      propagation_phase=self.propagation_phase, ge=self.ge, prob_type=self.prop_type)

        map_slit_plit = {}
        map_plit_slit = {}

        map_slit_plit[1] = []
        for str_symbol, plit, slit in atoms_list_for_mapping:
            map_plit_slit[plit] = slit
            map_plit_slit[-plit] = -slit
            if slit == 1:
                map_slit_plit[1].append(plit)
            else:
                map_slit_plit[slit] = plit
                map_slit_plit[-slit] = -plit

        facts = [max_plit] + map_slit_plit[1]
        to_watch = self.prop.getLiterals(*facts)

        for plit in to_watch:
            slit = map_plit_slit[plit]
            init.add_watch(literal=slit)

        self.map_plit_slit = map_plit_slit
        self.map_slit_plit = map_slit_plit

        # plit_list = self.prop.simplifyAtLevelZero()
        # slit_list = [map_plit_slit[plit] for plit in plit_list]
        
        #TODO: to finish the propagation at level 0


    def propagate(self, control: clingo.PropagateControl, changes: Sequence[int]) -> None:
        dl = control.assignment.decision_level
        for slit in changes:
            plit = self.map_slit_plit[slit]
            S_plit =  self.prop.onLiteralTrue(plit, dl)
            
            for plit in S_plit:
                R_plit = self.prop.getReasonForLiteral(plit)
                slit = self.map_plit_slit[plit]
                clause = [self.map_plit_slit[plit_r] for plit_r in R_plit]
                clause.append(slit)
                control.add_clause(clause)

    def undo(self, thread_id: int, assignment: clingo.Assignment, changes: Sequence[int]) -> None:
        plit_list = [self.map_slit_plit[slit] for slit in changes]
        self.prop.onLiteralsUndefined(*plit_list, wasp=False)