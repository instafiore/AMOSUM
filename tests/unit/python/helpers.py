"""helpers of the Python unit tests: the data files, a search on clingo or WASP, quiet output"""
import contextlib
import io
import os
import re

from fiore.program import Program

from amosum.amoclingo.python.runner import ClingoBackend
from amosum.core import search
from amosum.core.engine import Options
from amosum.core.program import read_aggregates
from amosum.core.progression import Progression

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
    """(best answer set, result, printed values) of a search with the Python propagator on clingo"""
    program, constraints = ground(name)
    backend = ClingoBackend(program, constraints, Options())
    out = io.StringIO()
    with contextlib.redirect_stdout(out):
        best, result = search.run(strategy, backend, backend.amomaximize, Progression(progression))
    values = [int(v) for v in re.findall(r"^Amomaximize Cost: (-?\d+) ", out.getvalue(), re.M)]
    return best, result, values, backend
