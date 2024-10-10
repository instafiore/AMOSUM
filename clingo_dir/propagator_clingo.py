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

'''
    Invariants: 
        Mapping from plit to slit
            from this:      #amosum{ W : colour_weight(C,W), col(X,C) [X] }
            to this:        group(col(X,C), W, X,0) :- col(X,C), colour_weight(C,W).
            # TODO: TO FINISH SYNTAX and SEMANTIC DEFINITION
        Indipendent literals:
            The aggregate set must not contains two equivalent literals
'''

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
        # print(f"atoms_list_for_mapping {atoms_list_for_mapping}")
        max_plit = 0
        for str_symbol, program_literal, solver_literal in atoms_list_for_mapping:
            if max_plit < program_literal:
                max_plit = program_literal

        self.atomNames = { str_symbol : program_literal for str_symbol, program_literal, solver_literal in atoms_list_for_mapping}

        self.prop = propagator_wasp.PropagatorWasp(atomsNames=self.atomNames, sys_parameters=self.sys_parameters,
                                      propagation_phase=self.propagation_phase, ge=self.ge, prob_type=self.prop_type)

        # This is a map for mapping each solver literal (slit) to its program literal(s) (plit).
        # Can happend that some solver literal has more than one program literal
        map_slit_plit_with_conflicts = {}

        # This map maps each solver literal (watched) to its unique program literal (watched)
        map_slit_plit_watched = {}

        map_plit_slit = {}

        for str_symbol, plit, slit in atoms_list_for_mapping:
            map_plit_slit[plit] = slit
            map_plit_slit[-plit] = -slit
            
            map_slit_plit_with_conflicts.setdefault(slit, [])
            map_slit_plit_with_conflicts.setdefault(-slit, [])
            map_slit_plit_with_conflicts[slit].append(plit)
            map_slit_plit_with_conflicts[-slit].append(-plit)

        lits = [max_plit] + map_slit_plit_with_conflicts.setdefault(1,[])
        to_watch = self.prop.getLiterals(*lits)

        for plit in to_watch:
            slit = map_plit_slit[plit]
            map_slit_plit_watched[slit] = plit
            # print(f" watch: {get_name(atomNames=self.atomNames, lit=plit)} plit: {plit} slit: {slit}")
            init.add_watch(literal=slit)

        self.map_plit_slit = map_plit_slit
        self.map_slit_plit_with_conflicts = map_slit_plit_with_conflicts
        self.map_slit_plit_watched = map_slit_plit_watched

        S_plit = self.prop.simplifyAtLevelZero()

        self.add_clause_propagated_lits(control=init, S_plit=S_plit)

        # print(f"atoms_list_for_mapping: {atoms_list_for_mapping}")
        # print(f"facts: {self.prop.facts}")
        # slit_prop_from_facts = [map_plit_slit[plit] for plit in S_plit]
        # print(f"slit_list_prop_0: {slit_prop_from_facts}")

    def add_clause_propagated_lits(self, control: clingo.PropagateControl | clingo.PropagateInit, S_plit):
        for plit in S_plit:
            R_plit = self.prop.getReasonForLiteral(plit)
            slit = self.map_plit_slit[plit]
            clause = [self.map_plit_slit[plit_r] for plit_r in R_plit]
            clause.append(slit)
            control.add_clause(clause)

    def propagate(self, control: clingo.PropagateControl, changes: Sequence[int]) -> None:
        dl = control.assignment.decision_level
        # print(f"size {len(changes)} dl {dl}")
        for slit in changes:
            plit = self.map_slit_plit_watched[slit]
            S_plit =  self.prop.onLiteralTrue(plit, dl)
            self.add_clause_propagated_lits(control=control, S_plit=S_plit)
            

    def undo(self, thread_id: int, assignment: clingo.Assignment, changes: Sequence[int]) -> None:
        plit_list = [self.map_slit_plit_watched[slit] for slit in changes]
        self.prop.onLiteralsUndefined(*plit_list, wasp=False)