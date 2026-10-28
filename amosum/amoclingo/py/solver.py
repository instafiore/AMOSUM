"""amosum on clingo (mirrors amoclingo/cpp/solver.h and solver.cpp): AmosumClingoSolver grounds the program, reads the
aggregates and registers one AmoclingoPropagator per aggregate; AmosumClingoBackend runs the searches of core/py/search
on it."""
import time
from typing import List, Optional

from fiore.model.program import Program
from fiore.model.solver import ClingoSolver

from amosum.amoclingo.py.propagator import AmoclingoPropagator
from amosum.core.py.program import AmosumClingoAssignment, read_aggregates
from amosum.core.py.propagator import AmosumOptions


class AmosumClingoSolver(ClingoSolver):

    def __init__(self, reason: str = "nomin", lazy: str = "false"):
        self.options = AmosumOptions(reason, lazy)
        self.constraints = []
        self.amosum_propagators: List[AmoclingoPropagator] = []
        self.maximizer: Optional[AmoclingoPropagator] = None

    def init(self, files: List[str]) -> None:
        """grounds the (rewritten) files, reads the aggregates and registers their propagators"""
        program = Program.ground(files)
        self.constraints, rules = read_aggregates(program)
        program.add_rules(rules)
        super().__init__(program)
        for c in self.constraints:
            p = AmoclingoPropagator(c, program, self.options)
            self.register(p)
            self.amosum_propagators.append(p)
            if c.is_amomaximize:
                self.maximizer = p

    def amomaximize(self):
        return self.maximizer.constraint if self.maximizer is not None else None


class AmosumClingoBackend:
    """the searches on clingo: one solve call per step, the bound of the #amomaximize read live by its propagator"""

    def __init__(self, solver: AmosumClingoSolver):
        self.solver = solver

    def amomaximize(self):
        return self.solver.amomaximize()

    def solve(self):
        amomaximize = self.amomaximize()
        program = self.solver.program

        def snapshot(model):
            def is_true(name):
                literal = program.literal(name)
                return literal is not None and model.is_true(literal.id)
            return {m.name: is_true(m.name) for g in amomaximize.groups for m in g.members} if amomaximize else None

        start = time.time()
        model, result = self.solver.solve(snapshot=snapshot)
        if not result.is_sat():
            return None, result
        truth = model.extra or {}
        return AmosumClingoAssignment(model.shown, amomaximize, lambda name: truth.get(name, False), time.time() - start), result

    def set_bound(self, bound: int) -> None:
        self.amomaximize().bound = bound

    def use_volatile_clauses(self) -> None:
        # only the #amomaximize propagator: the other constraints' bounds never change, their clauses stay learnt
        if self.solver.maximizer is not None:
            self.solver.maximizer.update_clause_type(True)
