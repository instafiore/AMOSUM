"""A clingo propagator running one PropagatorPBConstraintImplementation per solver thread (mirrors PropagatorPBConstraint
of fiore/cpp/fiore/model/propagatorPBConstraint.h).

Subclasses implement `init_first_time(init)`: called at the first init, it creates `self.implementations` (one per
thread) and adds the watches (with TOP, for the first propagation at level 0). A derivation d of an implementation is
added as the clause [d] + its reason. With a volatile clause type, the conflicts found at init are added by `check` at
the first fixpoint of the solve call: clauses added at init are permanent, while these hold only for the current bound.
"""
from typing import List, Sequence

import clingo

from fiore.model.propagator import ClingoPropagator
from fiore.utils.constants import BOTTOM, TOP


class PropagatorPBConstraint(ClingoPropagator):

    def __init__(self, name: str = "", volatile: bool = False):
        super().__init__(name, volatile)
        self.implementations: List = []
        self.first_time = True
        self.pending_init_clauses: List[List[List[int]]] = []

    # --- to implement ---
    def init_first_time(self, init: clingo.PropagateInit) -> None:
        raise NotImplementedError

    # --- clingo callbacks ---
    def init(self, init: clingo.PropagateInit) -> None:
        init.check_mode = clingo.PropagatorCheckMode.Off
        if self.first_time:
            self.first_time = False
            self.init_first_time(init)
        self.init_propagation(init)

    def init_propagation(self, init: clingo.PropagateInit) -> None:
        self.pending_init_clauses = [[] for _ in self.implementations]
        for ti, implementation in enumerate(self.implementations):
            for d in implementation.init_propagation():
                clause = self.clause_of(implementation, d)
                if self.is_volatile_clause_type():
                    self.pending_init_clauses[ti].append(clause)
                elif self.add_clause(init, clause):
                    return
        if any(self.pending_init_clauses):
            init.check_mode = clingo.PropagatorCheckMode.Fixpoint

    def propagate(self, control: clingo.PropagateControl, changes: Sequence[int]) -> None:
        # TOP (true from the start, watched by init_first_time): the first propagation at level 0
        force = TOP in changes
        if force:
            changes = [c for c in changes if c != TOP]
        implementation = self.implementations[control.thread_id]
        derived = implementation.on_literals_true(changes, control.assignment.decision_level, force)
        self.add_clauses_propagated_lit_ids(control, implementation, derived)

    def undo(self, thread_id: int, assignment: clingo.Assignment, changes: Sequence[int]) -> None:
        self.implementations[thread_id].on_literals_undefined(changes)

    def check(self, control: clingo.PropagateControl) -> None:
        ti = control.thread_id
        if ti >= len(self.pending_init_clauses) or not self.pending_init_clauses[ti]:
            return
        clauses, self.pending_init_clauses[ti] = self.pending_init_clauses[ti], []
        for clause in clauses:
            if self.add_clause(control, clause, volatile=True):
                return

    # --- clauses ---
    @staticmethod
    def clause_of(implementation, d: int) -> List[int]:
        return ([d] if d != BOTTOM else []) + list(implementation.get_reason_for_literal(d))

    def add_clauses_propagated_lit_ids(self, control: clingo.PropagateControl, implementation, derived: List[int]) -> bool:
        """True on a conflict"""
        for d in derived:
            if self.add_clause(control, self.clause_of(implementation, d)):
                return True
        return False
