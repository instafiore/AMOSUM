"""A clingo propagator dispatching to one engine per solver thread (port of fiore::PropagatorPBConstraint).

An engine works on solver literals and knows nothing about clingo:

    on_true(changes, dl, force) -> list of derived literals (BOTTOM: a conflict); force: first propagation at level 0
    undo(changes)
    reason(derived) -> the literals, false in the current assignment, that make the derivation valid
    init_check() -> derived literals before search, at every solve call (BOTTOM: the bound is violated already)

A derivation d is added as the clause [d] + reason(d) ([] + reason for BOTTOM).
"""
from typing import Callable, List, Optional, Sequence

import clingo

BOTTOM = 0      # never a solver literal: the conflict of an engine
TOP = 1         # the solver literal true from the start: build() watches it for the first propagation at level 0


class EnginePropagator:
    """Subclasses implement `build(init)`: called at the first init, it creates `self.engines` (one per thread) and
    adds the watches. `volatile` clauses are deleted after the solve call (a bound that can decrease between calls)."""

    def __init__(self):
        self.engines: List = []
        self.volatile = False
        self.first = True
        self.pending: List[List[List[int]]] = []       # volatile: the init clauses of each thread, added by check

    # --- to implement ---
    def build(self, init: clingo.PropagateInit) -> None:
        raise NotImplementedError

    # --- clingo callbacks ---
    def init(self, init: clingo.PropagateInit) -> None:
        init.check_mode = clingo.PropagatorCheckMode.Off
        if self.first:
            self.first = False
            self.build(init)
        self.pending = [[] for _ in self.engines]
        for ti, engine in enumerate(self.engines):
            for d in engine.init_check():
                clause = self._clause(engine, d)
                if self.volatile:
                    # a clause added here would be permanent, while it holds only for the current bound
                    self.pending[ti].append(clause)
                elif not init.add_clause(clause):
                    return
        if any(self.pending):
            init.check_mode = clingo.PropagatorCheckMode.Fixpoint

    def propagate(self, control: clingo.PropagateControl, changes: Sequence[int]) -> None:
        # literal 1 (true from the start, watched by build): the first propagation at level 0
        force = TOP in changes
        if force:
            changes = [c for c in changes if c != TOP]
        engine = self.engines[control.thread_id]
        derived = engine.on_true(changes, control.assignment.decision_level, force)
        self._add(control, engine, derived)

    def undo(self, thread_id: int, assignment: clingo.Assignment, changes: Sequence[int]) -> None:
        self.engines[thread_id].undo(changes)

    def check(self, control: clingo.PropagateControl) -> None:
        ti = control.thread_id
        if ti >= len(self.pending) or not self.pending[ti]:
            return
        clauses, self.pending[ti] = self.pending[ti], []
        for clause in clauses:
            if not control.add_clause(clause, tag=True) or not control.propagate():
                return

    # --- clauses ---
    @staticmethod
    def _clause(engine, d: int) -> List[int]:
        return ([d] if d != BOTTOM else []) + list(engine.reason(d))

    def _add(self, control: clingo.PropagateControl, engine, derived: List[int]) -> None:
        for d in derived:
            if not control.add_clause(self._clause(engine, d), tag=self.volatile) or not control.propagate():
                return
