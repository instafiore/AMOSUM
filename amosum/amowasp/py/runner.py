"""amowasp (mirrors amoclingo/py/runner.py on WASP): rewrites and grounds the files, then the search of --opt-strategy
on the #amomaximize; same output and exit codes as amoclingo-core."""
import tempfile
from typing import List

from fiore.clingo.utility_clingo import ClingoResult
from fiore.model.assignment import print_final

from amosum.amosumparser.rewrite import rewrite_all
from amosum.amowasp.py.solver import AmosumWaspBackend, AmosumWaspSolver
from amosum.core.py.search.binary import binary_search
from amosum.core.py.search.progression import Progression
from amosum.core.py.search.sat_unsat import sat_unsat
from amosum.core.py.search.unsat_sat import unsat_sat


class Runner:

    def run(self, inputs: List[str], wasp: str, reason: str = "nomin", lazy: str = "false", strategy: str = "sat-unsat",
            progression: str = "1") -> ClingoResult:
        steps = Progression(progression)
        best = None
        with tempfile.TemporaryDirectory(prefix="amowasp_") as tmp:
            solver = AmosumWaspSolver(wasp, reason, lazy)
            solver.init(rewrite_all(inputs, tmp), tmp)
            backend = AmosumWaspBackend(solver)
            try:
                if strategy == "sat-unsat" or backend.amomaximize() is None: best, result = sat_unsat(backend, steps)
                elif strategy == "unsat-sat": best, result = unsat_sat(backend, steps)
                else: best, result = binary_search(backend)     # --progression not used
            except KeyboardInterrupt:
                result = ClingoResult.satisfiable().set_unknown_optimum() if best is not None else ClingoResult.unknown()
        print_final(best, result)
        return result
