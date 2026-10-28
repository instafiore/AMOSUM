"""amowasp: amosum on WASP. Same options, output and exit codes as amoclingo-core.

The files are rewritten and grounded with clingo (Python API) to read and normalize the aggregates; clingo then writes
the ground program (smodels format) with the extra rules of the normalization, and WASP solves it with the plugin
amowasp_plugin. WASP is not incremental: each step of an #amomaximize search is a new WASP run with the new bound.
"""
import json
import os
import shutil
import subprocess
import sys
import tempfile
import time
from typing import List, Optional

from fiore.program import Program
from fiore.result import Result
from fiore.wasp import run_wasp

from amosum.core.constraint import model_value
from amosum.core.output import AnswerSet, print_final
from amosum.core.program import read_aggregates
from amosum.core.progression import Progression
from amosum.core import search
from amosum.rewrite import rewrite_all, shows_only_some_atoms

PLUGIN_DIRECTORY = os.path.dirname(os.path.abspath(__file__))
PLUGIN = "amowasp_plugin"


def clingo_executable() -> str:
    beside = os.path.join(os.path.dirname(sys.executable), "clingo")
    return beside if os.path.exists(beside) else (shutil.which("clingo") or "clingo")


def ground_program(files: List[str]) -> str:
    out = subprocess.run([clingo_executable(), *files, "--output=smodels", "--warn=none"], capture_output=True, text=True)
    if out.returncode not in (0, 10, 20, 30):
        raise RuntimeError(f"clingo failed: {out.stderr.strip()[-2000:]}")
    return out.stdout


class WaspBackend:
    def __init__(self, wasp: str, ground: str, constraints, reason: str, lazy: str, directory: str):
        self.wasp, self.ground, self.constraints = wasp, ground, constraints
        self.reason, self.lazy = reason, lazy
        self.config = os.path.join(directory, "amowasp_config.json")
        self.amomaximize = next((c for c in constraints if c.is_amomaximize), None)

    def solve(self):
        start = time.time()
        if self.constraints:
            with open(self.config, "w") as f:
                json.dump({"constraints": [c.to_json() for c in self.constraints], "reason": self.reason, "lazy": self.lazy}, f)
            model, status, _ = run_wasp(self.wasp, self.ground, PLUGIN_DIRECTORY, PLUGIN, env={"AMOWASP_CONFIG": self.config})
        else:
            model, status, _ = run_wasp(self.wasp, self.ground)
        if status == "UNSAT":
            return None, Result.unsatisfiable()
        if status != "SAT":
            return None, Result.unknown()
        atoms = set(model)
        value = model_value(self.amomaximize, lambda name: name in atoms) if self.amomaximize else None
        return AnswerSet(model, value, time.time() - start), Result.satisfiable()

    def set_bound(self, bound: int) -> None:
        self.amomaximize.bound = bound

    def use_volatile_clauses(self) -> None:
        pass                                    # every step is a new WASP run


def run(inputs: List[str], reason: str = "nomin", lazy: str = "false", strategy: str = search.SAT_UNSAT,
        progression: str = "1", wasp: Optional[str] = None) -> int:
    wasp = wasp or os.environ.get("WASP_EXE") or shutil.which("wasp")
    if not wasp:
        print("WASP not found: set WASP_EXE or put wasp on the PATH", file=sys.stderr)
        return 1
    print("amowasp version 3.0", flush=True)
    steps = Progression(progression)
    best = None
    with tempfile.TemporaryDirectory(prefix="amowasp_") as tmp:
        files = rewrite_all(inputs, tmp)
        program = Program.ground(files)
        constraints, rules = read_aggregates(program, show_members=shows_only_some_atoms(files))
        extra = os.path.join(tmp, "amowasp_extra.lp")
        with open(extra, "w") as f:
            f.write(rules)
        backend = WaspBackend(wasp, ground_program(files + [extra]), constraints, reason, lazy, tmp)
        try:
            best, result = search.run(strategy, backend, backend.amomaximize, steps)
        except KeyboardInterrupt:
            result = Result.satisfiable().set_unknown_optimum() if best is not None else Result.unknown()
    print_final(best, result)
    return result.exit_code()
