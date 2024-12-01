#!/home/s.fiorentino/miniconda3/bin/python3
import sys
import os
from typing import Sequence
# adding the root path
import clingo
from clingo.symbol import Number, Function
from clingo.control import Control
from clingo.symbol import Function

sys.path.append(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
from amosum import *
import amosum
'''
    Invariants: 
'''

class PropagatorClingo(clingo.Propagator):

    def __init__(self, sys_parameters: dict[str: str], propagation_phase: Callable[[Group, AmoSumPropagator], List[int]], ge: bool, prop_type: str) -> None:
        super().__init__()
        self.sys_parameters = sys_parameters
        self.propagation_phase = propagation_phase
        self.ge = ge
        self.prop_type = prop_type
        self.solver = AmoSumPropagator.CLINGO

    def init(self, init: clingo.PropagateInit) -> None:

        atoms_list_for_mapping = [(str(a.symbol), a.literal, init.solver_literal(a.literal)) for a in init.symbolic_atoms]
        nt = init.number_of_threads
        # debug(f"number of threads {nt}", force_print = True)
        # debug(f"atoms_list_for_mapping {atoms_list_fosr_mapping}", force_print = False)
        debug("TRY", force_print=True)
        max_plit = 0
        for str_symbol, program_literal, solver_literal in atoms_list_for_mapping:
            if max_plit < program_literal:
                max_plit = program_literal

        self.atomNames = { str_symbol : program_literal for str_symbol, program_literal, solver_literal in atoms_list_for_mapping}

        self.propagators = [amosum.AmoSumPropagator(atomsNames=self.atomNames, sys_parameters=self.sys_parameters,
                                      propagation_phase=self.propagation_phase, ge=self.ge, choice_cons=self.prop_type, solver=AmoSumPropagator.CLINGO) for i in range(nt)]

        # This is a map for mapping each solver literal (slit) to its program literal(s) (plit).
        # Can happend that some solver literal has more than one program literal
        map_slit_plit = {}

        # This map maps each solver literal (watched) to its program literals (watched)
        map_slit_plit_watched = {}

        map_plit_slit = {}

        for str_symbol, plit, slit in atoms_list_for_mapping:
            map_plit_slit[plit] = slit
            map_plit_slit[-plit] = -slit
            
            map_slit_plit.setdefault(slit, [])
            map_slit_plit.setdefault(-slit, [])
            map_slit_plit[slit].append(plit)
            map_slit_plit[-slit].append(-plit)

        lits = [max_plit] + map_slit_plit.get(1,[])
        for i in range(nt):
            to_watch_plit = self.propagators[i].getLiterals(*lits)

        for plit in to_watch_plit:
            slit = map_plit_slit[plit]
            map_slit_plit_watched.setdefault(slit,[])
            map_slit_plit_watched[slit].append(plit)
            init.add_watch(literal=slit)

        self.map_plit_slit = map_plit_slit
        self.map_slit_plit = map_slit_plit
        self.map_slit_plit_watched = map_slit_plit_watched

        debug("Propagation ad level 0 started")
        for i in range(nt):
            S_plit = self.propagators[i].simplifyAtLevelZero(delete_lits=True)

        if S_plit == [1] or self.add_clauses_propagated_lits(control=init, S_plit=S_plit, dl = 0):
            # adding empty clause
            init.add_clause([])
            return

    def add_clauses_propagated_lits(self, control: clingo.PropagateControl | clingo.PropagateInit, S_plit, dl):
        td = 0 if isinstance(control, clingo.PropagateInit) else control.thread_id
        prop = self.propagators[td]
        for plit in S_plit:
            R_plit = prop.getReasonForLiteral(plit)
            
            slit   = self.map_plit_slit[plit]
            # first part of the clause is the reason
            clause = [self.map_plit_slit[plit_r] for plit_r in R_plit] 
            # the last literal is the implied literal (undefined)
            clause.append(slit)
            if not control.add_clause(clause) or not control.propagate():
                # propagation must return immediately, a conflict has been raised
                print_clause(propagator=self, clause=clause, conflict=True)
                return True
        return False
    

    def propagate(self, control: clingo.PropagateControl, changes: Sequence[int]) -> None:
        dl = control.assignment.decision_level
        td = 0 if dl == 0 else control.thread_id
        prop = self.propagators[td]
        print_propagate(self, changes=changes, control=control, dl=dl)
        for slit in changes:
            plit_list = self.map_slit_plit_watched[slit]
            for plit in plit_list:
                # propagated plits
                S_plit = []
                # propagating program literal
                try:
                    S_plit = prop.onLiteralTrue(plit, dl)
                except Exception as e:
                    debug(e, force_print=True)
                    raise e
                # adding clauses for propagated literals S_plit
                if self.add_clauses_propagated_lits(control=control, S_plit=S_plit, dl = dl):
                    # Conflict added hence propagation has to stop
                    return 

    def undo(self, thread_id: int, assignment: clingo.Assignment, changes: Sequence[int]) -> None:
        prop = self.propagators[thread_id]
        plit_list = []
        for slit in changes:
            for plit in self.map_slit_plit_watched[slit]:
                plit_list.append(plit)
        print_undo(self, changes=changes, thread_id=thread_id)
        try:
            prop.onLiteralsUndefined(*plit_list, wasp=False)
        except Exception as e:
            debug(e, force_print=True)
            raise e

    def compute_changes_str(self, changes, thread_id):
        changes_str = []
        for slit in changes:
             for plit in self.map_slit_plit_watched[slit]:
                 changes_str.append((get_name(atomNames=self.atomNames, lit = plit), self.map_slit_plit_watched[slit], slit))
        return changes_str
    
    
    def createClingoInterpretation(self, slit_list: List[int], assignment: clingo.Assignment):
        I_clingo = SymmetricFunction(self.propagators[0].N)
        for slit in slit_list:
            plit = self.map_slit_plit_watched[slit]
            I_clingo[plit] = assignment.value(slit)
        return I_clingo