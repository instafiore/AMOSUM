"""helpers of the Python unit tests: the data files, a search on clingo, quiet output"""
import contextlib
import io
import os
import re

from fiore.model.program import Program

from amosum.amoclingo.py.solver import AmosumClingoBackend, AmosumClingoSolver
from amosum.core.py.program import read_aggregates
from amosum.core.py.search.binary import binary_search
from amosum.core.py.search.progression import Progression
from amosum.core.py.search.sat_unsat import sat_unsat
from amosum.core.py.search.unsat_sat import unsat_sat

DATA = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "data")

STRATEGIES = ["sat-unsat", "unsat-sat", "binary"]
PROGRESSIONS = ["1", "2", "2^n", "n^2"]


def data(name: str, extension: str = "lp") -> str:
    return os.path.join(DATA, f"{name}.{extension}")


def configurations():
    """every (strategy, progression); binary does not use the progression: once"""
    return [(s, p) for s in STRATEGIES for p in (["1"] if s == "binary" else PROGRESSIONS)]


def ground(name: str):
    """data/<name>.lp (already rewritten by the parser) grounded, with its aggregates read and normalized"""
    program = Program.ground([data(name)])
    constraints, rules = read_aggregates(program)
    program.add_rules(rules)
    return program, constraints


def clingo_search(name: str, strategy: str = "sat-unsat", progression: str = "1"):
    """(best answer set, result, printed values, solver) of a search with the Python propagator on clingo"""
    solver = AmosumClingoSolver()
    solver.init([data(name)])
    backend = AmosumClingoBackend(solver)
    steps = Progression(progression)
    out = io.StringIO()
    with contextlib.redirect_stdout(out):
        if strategy == "sat-unsat": best, result = sat_unsat(backend, steps)
        elif strategy == "unsat-sat": best, result = unsat_sat(backend, steps)
        else: best, result = binary_search(backend)
    values = [int(v) for v in re.findall(r"^Amomaximize Cost: (-?\d+) ", out.getvalue(), re.M)]
    return best, result, values, solver
