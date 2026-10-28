"""amoclingo --lang py: the Python version of amoclingo-core (clingo's Python API, the propagator of amosum.core).

Same options, output and exit codes as amoclingo-core: the files are rewritten, grounded, the aggregates read and
normalized (amosum.core.program), one propagator per aggregate, then the search of --opt-strategy on #amomaximize.
"""
import tempfile
import time
from typing import List, Optional

from fiore.program import Program
from fiore.solver import ClingoSolver

from amosum.amoclingo.python.propagator import AmosumPropagator
from amosum.core.constraint import model_value
from amosum.core.engine import Options
from amosum.core.output import AnswerSet, print_final
from amosum.core.program import read_aggregates
from amosum.core.progression import Progression
from amosum.core import search
from amosum.rewrite import rewrite_all


class ClingoBackend:
    """one solve call per search step, on one Control: the bound of the #amomaximize is read live by its engines"""

    def __init__(self, program: Program, constraints, options: Options):
        self.program = program
        self.solver = ClingoSolver(program)
        self.maximizer: Optional[AmosumPropagator] = None
        self.amomaximize = next((c for c in constraints if c.is_amomaximize), None)
        for c in constraints:
            p = AmosumPropagator(c, program, options)
            self.solver.register(p)
            if c.is_amomaximize:
                self.maximizer = p

    def _value(self, model):
        if self.amomaximize is None:
            return None
        def is_true(name):
            literal = self.program.literal(name)
            return literal is not None and model.is_true(literal.id)
        return model_value(self.amomaximize, is_true)

    def solve(self):
        start = time.time()
        model, result = self.solver.solve(snapshot=self._value)
        if not result.is_sat():
            return None, result
        return AnswerSet(model.shown, model.extra, time.time() - start), result

    def set_bound(self, bound: int) -> None:
        self.amomaximize.bound = bound

    def use_volatile_clauses(self) -> None:
        if self.maximizer is not None:
            self.maximizer.volatile = True


def run(inputs: List[str], reason: str = "nomin", lazy: str = "false", strategy: str = search.SAT_UNSAT,
        progression: str = "1") -> int:
    """the exit code: 10 SAT, 20 UNSAT, 30 optimum, 29 interrupted with an answer set"""
    print("amoclingo version 3.0 (python)", flush=True)
    steps = Progression(progression)
    with tempfile.TemporaryDirectory(prefix="amoclingo_py_") as tmp:
        program = Program.ground(rewrite_all(inputs, tmp))
    constraints, rules = read_aggregates(program)
    program.add_rules(rules)
    backend = ClingoBackend(program, constraints, Options(reason, lazy))
    best = None
    try:
        best, result = search.run(strategy, backend, backend.amomaximize, steps)
    except KeyboardInterrupt:
        from fiore.result import Result
        result = Result.satisfiable().set_unknown_optimum() if best is not None else Result.unknown()
    print_final(best, result)
    return result.exit_code()
