#!/home/s.fiorentino/miniconda3/bin/python3
import sys
import os
# adding the root path
sys.path.append(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
import clingo
from propagator import *
from clingo.symbol import Number, Function
from clingo.control import Control
from clingo.symbol import Function
from clingo.propagator import Propagator

class GeAmo(Propagator):

    def init(self, init: clingo.PropagateInit) -> None:
        atoms = list(init.symbolic_atoms)
        atoms_list_for_mapping = [(str(a.symbol), a.literal, init.solver_literal(a.literal)) for a in init.symbolic_atoms]
        print(atoms_list_for_mapping)

        atomNames = { str_symbol : program_literal for str_symbol, program_literal, solver_literal in atoms_list_for_mapping}
        
        # getLiterals()


    def propagate(self, control: clingo.PropagateControl, changes: sys.Sequence[int]) -> None:
        pass
        #     # onLiteralTrue()

        #     # getReasonForLiteral()

        #     # addClause()    

    def undo(self, thread_id: int, assignment: clingo.Assignment, changes: sys.Sequence[int]) -> None:
        pass
        # onLiteralsUndefined()