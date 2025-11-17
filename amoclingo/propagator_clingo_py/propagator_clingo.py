#!/home/s.fiorentino/miniconda3/bin/python3
import sys
import os
from typing import Sequence
# adding the root path
import clingo
from clingo.symbol import Number, Function
from clingo.control import Control
from clingo.symbol import Function

from amosum_initializer import AmoSumInitializer

sys.path.append(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
from amosum import *
import amosum
'''
    Invariants: 
'''
import time 
from amoclingo.propagator_clingo_py.propagator_clingo_initializer import *

class PropagatorClingo(clingo.Propagator):

    def __init__(self, sys_parameters: dict[str: str], propagation_phase: Callable[[Group, AmoSumPropagator], List[int]], ge: bool, choice_cons: str) -> None:
        super().__init__()
        self.sys_parameters = sys_parameters
        self.propagation_phase = propagation_phase
        self.ge = ge
        self.choice_cons = choice_cons
        self.solver = AmoSumPropagator.CLINGO

    def init(self, _init: clingo.PropagateInit) -> None:
        PropagatorClingoInitializer.get_instance().init(_init, self)

        self.propagators = []
        for i in range(PropagatorClingoInitializer.get_instance().nt):
                propagator = amosum.AmoSumPropagator(atomNames=self.atomNames, sys_parameters=self.sys_parameters,
                                      propagation_phase=self.propagation_phase, ge=self.ge, choice_cons=self.choice_cons, solver=AmoSumPropagator.CLINGO)
                propagator.map_plit_slit = self.map_plit_slit
                self.propagators.append(propagator)

        for i in range(PropagatorClingoInitializer.get_instance().nt): to_watch_plit = AmoSumInitializer.get_instance().getLiterals(PropagatorClingoInitializer.get_instance().lits, self.propagators[i])
        
        # arrived here 
        map_slit_plit_watched = {}
        for plit in to_watch_plit:
            slit = self.map_plit_slit[plit]
            map_slit_plit_watched.setdefault(slit,[])
            map_slit_plit_watched[slit].append(plit)
            # debug(f"watching: {slit} {get_name(self.atomNames,plit)}", force_print=True)
            _init.add_watch(literal=slit)
            
            
        self.map_slit_plit_watched = map_slit_plit_watched

        # debug("Propagation ad level 0 started")
        for i in range(PropagatorClingoInitializer.get_instance().nt):
            S_plit = self.propagators[i].simplifyAtLevelZero(delete_lits=True)
            
        # print_derivation(atomNames=self.atomNames, S=S_plit)
        if S_plit == [1] or self.add_clauses_propagated_lits(control=_init, S_plit=S_plit, dl = 0) or not _init.propagate():
            # adding empty clause
            _init.add_clause([])
            return
        
        

    def add_clauses_propagated_lits(self, control: clingo.PropagateControl | clingo.PropagateInit, S_plit, dl):
        td = 0 if isinstance(control, clingo.PropagateInit) else control.thread_id
        prop = self.propagators[td]

        for si in range(len(S_plit)):
            plit = S_plit[si]
            try:
                R_plit = prop.getReasonForLiteral(plit)

                slit  = self.map_plit_slit[plit]
                # first part of the clause is the reason
                clause = [self.map_plit_slit[plit_r] for plit_r in R_plit] 
                
                # the last literal is the implied literal (undefined or false)
                clause.append(slit)
                
                if not control.add_clause(clause):
                    # propagation must return immediately, a conflict has been raised
                    return True
                
                if not control.propagate():
                    return True
            except Exception as e:
                raise e
        return False
    

    def propagate(self, control: clingo.PropagateControl, changes: Sequence[int]) -> None:
        try:
            dl = control.assignment.decision_level
            td = 0 if dl == 0 else control.thread_id
            prop = self.propagators[td]
            prop.control = control
            # print_propagate(self, changes=changes, control=control, dl=dl, force_print=True)

            for ci in range(len(changes)):
                slit = changes[ci]
                plit_list = self.map_slit_plit_watched[slit]
                for plit in plit_list:
                    # propagated plits
                    S_plit = []
                    # propagating program literal
                    S_plit = prop.onLiteralTrue(plit, dl)
                    # adding clauses for propagated literals S_plit
                    if self.add_clauses_propagated_lits(control=control, S_plit=S_plit, dl = dl):
                        return 
            
            
        except Exception as e:
            debug(e, force_print=True)
            tb = e.__traceback__
            while tb:
                print(f"File: {tb.tb_frame.f_code.co_filename}, Line: {tb.tb_lineno}, Function: {tb.tb_frame.f_code.co_name}", file=sys.stderr)
                tb = tb.tb_next
            raise e

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
            tb = e.__traceback__
            while tb:
                print(f"File: {tb.tb_frame.f_code.co_filename}, Line: {tb.tb_lineno}, Function: {tb.tb_frame.f_code.co_name}", file=sys.stderr)
                tb = tb.tb_next
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