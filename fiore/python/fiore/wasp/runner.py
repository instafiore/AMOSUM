"""Runs the WASP binary on a ground program (smodels, as clingo --output=smodels) and reads its models (no C++
counterpart; pure Python)."""
import os
import re
import subprocess
from typing import List, Optional, Sequence, Tuple

MODEL = re.compile(r"^\{(.*)\}\s*$")

_running: Optional[subprocess.Popen] = None        # the WASP run in progress (killed by stop_wasp)


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
    global _running
    command = [exe, *arguments]
    if plugin is not None:
        command += ["--interpreter=python", f"--script-directory={script_directory}", f"--plugins-file={plugin}"]
    proc = subprocess.Popen(command, stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True,
                            env={**os.environ, **(env or {})})
    _running = proc
    try:
        stdout, stderr = proc.communicate(ground_program, timeout=timeout)
    except subprocess.TimeoutExpired:
        proc.kill()
        proc.communicate()
        return None, "UNKNOWN", -1
    finally:
        _running = None
    model = None
    for line in stdout.splitlines():
        m = MODEL.match(line.strip())
        if m:
            model = split_atoms(m.group(1))
    if model is not None:
        return model, "SAT", proc.returncode
    if "INCOHERENT" in stdout:
        return None, "UNSAT", proc.returncode
    if proc.returncode != 0 and stderr.strip():
        raise RuntimeError(f"WASP failed ({proc.returncode}): {stderr.strip()[-2000:]}")
    return None, "UNKNOWN", proc.returncode


def stop_wasp() -> None:
    """kills the WASP run in progress, if any (when the run is interrupted: WASP may not have got the signal)"""
    proc = _running
    if proc is not None and proc.poll() is None:
        proc.kill()
