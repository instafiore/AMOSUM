"""amosum on WASP (mirrors amoclingo/py/solver.py for WASP; no C++ counterpart): AmosumWaspSolver grounds the program
with clingo (Python API) to read and normalize the aggregates, then clingo writes the ground program (smodels) with the
extra rules of the normalization; AmosumWaspBackend runs the searches of core/py/search: each solve call is a WASP run
with the plugin amowasp_plugin (WASP is not incremental: the bound changes between runs).
"""
import json
import os
import shutil
import subprocess
import sys
import time
from typing import List, Optional

from fiore.clingo.utility_clingo import ClingoResult
from fiore.model.program import Program
from fiore.wasp.runner import run_wasp

from amosum.amosumparser.rewrite import shows_only_some_atoms
from amosum.core.py.program import AmosumClingoAssignment, read_aggregates

PLUGIN_DIRECTORY = os.path.join(os.path.dirname(os.path.abspath(__file__)), "plugin")
PLUGIN = "amowasp_plugin"


def clingo_executable() -> str:
    beside = os.path.join(os.path.dirname(sys.executable), "clingo")
    return beside if os.path.exists(beside) else (shutil.which("clingo") or "clingo")


class AmosumWaspSolver:

    def __init__(self, wasp: str, reason: str = "nomin", lazy: str = "false"):
        self.wasp, self.reason, self.lazy = wasp, reason, lazy
        self.constraints = []
        self.ground_program = ""
        self.config = None

    def init(self, files: List[str], directory: str) -> None:
        """grounds the (rewritten) files: the aggregates, then the smodels program with the normalization's rules"""
        program = Program.ground(files)
        self.constraints, rules = read_aggregates(program, show_members=shows_only_some_atoms(files))
        extra = os.path.join(directory, "amowasp_extra.lp")
        with open(extra, "w") as f:
            f.write(rules)
        out = subprocess.run([clingo_executable(), *files, extra, "--output=smodels", "--warn=none"], capture_output=True, text=True)
        if out.returncode not in (0, 10, 20, 30):
            raise RuntimeError(f"clingo failed: {out.stderr.strip()[-2000:]}")
        self.ground_program = out.stdout
        self.config = os.path.join(directory, "amowasp_config.json")

    def amomaximize(self):
        return next((c for c in self.constraints if c.is_amomaximize), None)

    def solve(self):
        """one WASP run: (the atoms of the model or None, 'SAT' | 'UNSAT' | 'UNKNOWN')"""
        if not self.constraints:
            model, status, _ = run_wasp(self.wasp, self.ground_program)
            return model, status
        with open(self.config, "w") as f:
            json.dump({"constraints": [c.to_json() for c in self.constraints], "reason": self.reason, "lazy": self.lazy}, f)
        model, status, _ = run_wasp(self.wasp, self.ground_program, PLUGIN_DIRECTORY, PLUGIN, env={"AMOWASP_CONFIG": self.config})
        return model, status


class AmosumWaspBackend:
    """the searches on WASP: a WASP run per step, with the current bound of the #amomaximize"""

    def __init__(self, solver: AmosumWaspSolver):
        self.solver = solver

    def amomaximize(self):
        return self.solver.amomaximize()

    def solve(self):
        start = time.time()
        model, status = self.solver.solve()
        if status == "UNSAT":
            return None, ClingoResult.unsatisfiable()
        if status != "SAT":
            return None, ClingoResult.unknown()
        atoms = set(model)
        return AmosumClingoAssignment(model, self.amomaximize(), lambda name: name in atoms, time.time() - start), ClingoResult.satisfiable()

    def set_bound(self, bound: int) -> None:
        self.amomaximize().bound = bound

    def use_volatile_clauses(self) -> None:
        pass                                    # every step is a new WASP run
