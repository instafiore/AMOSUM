"""WASP: the plugin protocol and a runner for the binary (pure Python: the plugin runs inside WASP's interpreter).

WASP calls the functions of a plugin module (--plugins-file=<module>):
    getLiterals(n, *level0)       n variables, the literals fixed at level 0; returns the literals to watch
    simplifyAtLevelZero()         returns unit clauses (a literal and its negation: inconsistent)
    onLiteralTrue(lit, dl)        a watched literal is true; returns the derived literals (a false one: a conflict)
    getReasonForLiteral(lit)      the literals, all false, such that lit or them is a clause
    onLiteralsUndefined(dl, *lits)
and fills the module attribute atomNames (atom name -> variable). `install` binds these functions to a WaspPlugin.
"""
import os
import re
import subprocess
from typing import Callable, Dict, List, Optional, Sequence, Tuple

BOTTOM = 0      # the conflict of an engine (as fiore.propagator.BOTTOM)


class WaspPlugin:
    """`build(atom_names, level0)` (level0: the literals fixed at level 0) returns (engine, watched literals, unit
    literals at level 0). The engine is as in fiore.propagator, with on_true(changes, dl, force=False): force runs the
    first propagation at level 0."""

    def __init__(self, atom_names: Dict[str, int], build: Callable):
        self.atom_names = atom_names
        self.build = build
        self.engine = None
        self.units: List[int] = []
        self.level0: List[int] = []
        self.reasons: Dict[int, List[int]] = {}
        self.any_variable = 1

    def get_literals(self, *lits) -> List[int]:
        self.level0 = list(lits[1:])
        self.engine, watched, self.units = self.build(self.atom_names, set(self.level0))
        if watched:
            self.any_variable = abs(watched[0])
        return watched

    def simplify_at_level_zero(self) -> List[int]:
        units = list(self.units)
        derived = self.engine.init_check() + self.engine.on_true([], 0, force=True)
        if BOTTOM in derived:
            return [self.any_variable, -self.any_variable]          # inconsistent at level 0
        return units + derived

    def on_literal_true(self, lit: int, dl: int) -> List[int]:
        derived = self.engine.on_true([lit], dl)
        out = []
        for d in derived:
            reason = list(self.engine.reason(d))
            if d == BOTTOM:
                # a conflict: WASP wants a false literal and its reason, with a literal of the current level in the
                # reason; not(lit) is false (lit was just set true) and of this level: the conflict literal, and in the
                # reason too (with lazy propagation the other literals may all be of lower levels), as the old version
                d = -lit
                if d not in reason:
                    reason.append(d)
            elif dl > 0 and -lit not in reason and d != -lit:
                # WASP wants a literal of the current decision level in every reason (clingo does not): not(lit) is
                # false and of this level; adding a false literal keeps the clause valid (only weaker)
                reason.append(-lit)
            self.reasons[d] = reason
            out.append(d)
        return out

    def get_reason_for_literal(self, lit: int) -> List[int]:
        return self.reasons.get(lit, [])

    def on_literals_undefined(self, *lits) -> None:
        self.engine.undo(lits[1:])


def install(module_globals: dict, plugin: WaspPlugin) -> None:
    """Binds WASP's plugin functions in a module to `plugin`."""
    module_globals["getLiterals"] = plugin.get_literals
    module_globals["simplifyAtLevelZero"] = plugin.simplify_at_level_zero
    module_globals["onLiteralTrue"] = plugin.on_literal_true
    module_globals["getReasonForLiteral"] = plugin.get_reason_for_literal
    module_globals["onLiteralsUndefined"] = plugin.on_literals_undefined


# --- runner ---

MODEL = re.compile(r"^\{(.*)\}\s*$")


def split_atoms(text: str) -> List[str]:
    """The atoms of a WASP model line '{a, f(b,c), d}' (commas inside terms are not separators)."""
    atoms, depth, current = [], 0, ""
    for ch in text:
        if ch in "([": depth += 1
        elif ch in ")]": depth -= 1
        if ch == "," and depth == 0:
            atoms.append(current.strip()); current = ""
        else:
            current += ch
    if current.strip():
        atoms.append(current.strip())
    return atoms


def run_wasp(exe: str, ground_program: str, script_directory: Optional[str] = None, plugin: Optional[str] = None,
             arguments: Sequence[str] = (), env: Optional[dict] = None,
             timeout: Optional[float] = None) -> Tuple[Optional[List[str]], str, int]:
    """Runs WASP on a ground program (smodels format, as clingo --output=smodels). Returns (the atoms of the last
    model or None, 'SAT' | 'UNSAT' | 'UNKNOWN', WASP's exit code)."""
    command = [exe, *arguments]
    if plugin is not None:
        command += ["--interpreter=python", f"--script-directory={script_directory}", f"--plugins-file={plugin}"]
    try:
        out = subprocess.run(command, input=ground_program, capture_output=True, text=True, timeout=timeout,
                             env={**os.environ, **(env or {})})
    except subprocess.TimeoutExpired:
        return None, "UNKNOWN", -1
    model = None
    for line in out.stdout.splitlines():
        m = MODEL.match(line.strip())
        if m:
            model = split_atoms(m.group(1))
    if model is not None:
        return model, "SAT", out.returncode
    if "INCOHERENT" in out.stdout:
        return None, "UNSAT", out.returncode
    if out.returncode != 0 and out.stderr.strip():
        raise RuntimeError(f"WASP failed ({out.returncode}): {out.stderr.strip()[-2000:]}")
    return None, "UNKNOWN", out.returncode
