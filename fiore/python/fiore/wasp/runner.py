"""Runs the WASP binary on a ground program (smodels, as clingo --output=smodels) and reads its models (no C++
counterpart; pure Python)."""
import os
import re
import subprocess
from typing import List, Optional, Sequence, Tuple

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
