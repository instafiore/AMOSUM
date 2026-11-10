"""amoclingo --lang py (mirrors amoclingo/cpp/runner.h and runner.cpp): rewrites and grounds the files, then the search
of --opt-strategy on the #amomaximize; same output and exit codes as amoclingo-core."""
import tempfile
from typing import List

from fiore.clingo.utility_clingo import ClingoResult
from fiore.model.assignment import set_interrupt_printer

from amosum.amoclingo.py.solver import AmosumClingoBackend, AmosumClingoSolver
from amosum.amosumparser.rewrite import rewrite_all
from amosum.core.py.program import set_print_mode
from amosum.core.py.search.algorithms import print_last
from amosum.core.py.search.binary import binary_search
from amosum.core.py.search.progression import Progression
from amosum.core.py.search.sat_unsat import sat_unsat
from amosum.core.py.search.unsat_sat import unsat_sat


class Runner:

    def run(self, inputs: List[str], reason: str = "nomin", lazy: str = "false", strategy: str = "sat-unsat",
            progression: str = "1", verbose: str = "each-model") -> ClingoResult:
        set_print_mode(verbose)
        set_interrupt_printer(print_last)       # an interrupted run ends with the same last line
        steps = Progression(progression)
        solver = AmosumClingoSolver(reason, lazy)
        with tempfile.TemporaryDirectory(prefix="amoclingo_py_") as tmp:
            solver.init(rewrite_all(inputs, tmp))
        backend = AmosumClingoBackend(solver)
        # an interruption (SIGINT, SIGTERM) is handled by fiore.model.assignment.on_interrupt, installed by main
        if strategy == "sat-unsat" or backend.amomaximize() is None: best, result = sat_unsat(backend, steps)
        elif strategy == "unsat-sat": best, result = unsat_sat(backend, steps)
        else: best, result = binary_search(backend)         # --progression not used
        print_last(best, result)
        return result
