#!/home/s.fiorentino/miniconda3/bin/python3
import sys
import os
from typing import Sequence
# adding the root path
sys.path.append(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
import clingo
from propagator import *
import propagator
from clingo.symbol import Number, Function
from clingo.control import Control
from clingo.symbol import Function
from clingo.propagator import Propagator

class GeAmo(clingo.Propagator):

    def __init__(self, sys_parameters: dict[str: str], propagation_phase: Callable[[Group, Propagator], List[int]]) -> None:
        super().__init__()
        self.sys_parameters = sys_parameters
        self.propagation_phase = propagation_phase

    def init(self, init: clingo.PropagateInit) -> None:
        atoms = list(init.symbolic_atoms)
        atoms_list_for_mapping = [(str(a.symbol), a.literal, init.solver_literal(a.literal)) for a in init.symbolic_atoms]
        print(atoms_list_for_mapping)

        self.atomNames = { str_symbol : program_literal for str_symbol, program_literal, solver_literal in atoms_list_for_mapping}

        self.prop = propagator.Propagator(atomsNames=self.atomNames, sys_parameters=self.sys_parameters,
                                      propagation_phase=self.propagation_phase, ge=True, prob_type="AMO")

        map_slit_plit = PerfectHash(len(self.atomNames), default=None)
        map_plit_slit = PerfectHash(len(self.atomNames), default=None)

        map_slit_plit[1] = []
        for str_symbol, plit, slit in atoms_list_for_mapping:
            map_plit_slit[plit] = slit
            if slit == 1:
                map_slit_plit[1].append(plit)
            else:
                map_slit_plit[slit] = plit

        facts = map_slit_plit[1]
        to_watch = self.prop.getLiterals(*facts)

        for plit in to_watch:
            slit = map_plit_slit[plit]
            init.add_watch(literal=slit)

    def propagate(self, control: clingo.PropagateControl, changes: Sequence[int]) -> None:
        pass
        #     # onLiteralTrue()

        #     # getReasonForLiteral()

        #     # addClause()    

    def undo(self, thread_id: int, assignment: clingo.Assignment, changes: Sequence[int]) -> None:
        pass
        # onLiteralsUndefined()