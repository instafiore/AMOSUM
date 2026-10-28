"""Solve calls on a Program, one model each (mirrors the parts of ClingoSolver of fiore/cpp/fiore/model/solver.h
that a search needs)."""
from typing import Any, Callable, Optional, Tuple

import clingo

from fiore.clingo.utility_clingo import ClingoResult
from fiore.model.program import Program


class Model:
    """What is kept of a clingo model after its solve call: the shown atoms and what `snapshot` computed from it."""

    def __init__(self, shown, extra):
        self.shown = shown          # list of the shown atoms, as text
        self.extra = extra


class ClingoSolver:

    def __init__(self, program: Program):
        self.program = program
        self.propagators = []

    def register(self, propagator) -> None:
        self.program.ctl.register_propagator(propagator)
        self.propagators.append(propagator)

    def solve(self, snapshot: Optional[Callable[[clingo.Model], Any]] = None) -> Tuple[Optional[Model], ClingoResult]:
        """One solve call: the first model (with snapshot(model), read while the model is valid) and the result."""
        found = []

        def on_model(model: clingo.Model):
            shown = [str(s) for s in model.symbols(shown=True)]
            found.append(Model(shown, snapshot(model) if snapshot else None))
            return False                    # one model

        result = self.program.ctl.solve(on_model=on_model)
        if result.satisfiable:
            return found[-1], ClingoResult.satisfiable()
        if result.unsatisfiable:
            return None, ClingoResult.unsatisfiable()
        return None, ClingoResult.unknown()
