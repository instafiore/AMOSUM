"""amowasp: amosum on WASP (the WASP executable: --wasp, or the environment variable WASP_EXE).

    amowasp encoding.asp instance.asp [--reason ...] [--lazy ...] [--opt-strategy ...] [--progression ...]

Same options (those that change the search), output and exit codes as amoclingo.
"""
import argparse
import os
import re
import sys

from amosum.amowasp.python.runner import run as runner


def lazyValue(value: str) -> str:
    if not re.fullmatch(r"false|true|hybrid|0(\.\d+)?|1(\.0+)?", value):
        raise argparse.ArgumentTypeError(f"{value}: expected false, true, hybrid, or a float in [0,1]")
    return value


def parseArguments() -> argparse.Namespace:
    parser = argparse.ArgumentParser(prog="amowasp", description="WASP with #amosum, #eosum and #amomaximize", allow_abbrev=False)
    parser.add_argument("inputs", nargs="+", metavar="asp-input", help="ASP files (encoding, instance, ...)")
    parser.add_argument("--reason", choices=["nomin", "ijcai", "min", "minfly", "cmin"], default="nomin",
                        help="Reasons of the >= derivations [nomin]")
    parser.add_argument("--lazy", type=lazyValue, default="false",
                        help="When to propagate: false, true, hybrid, or a float in [0,1] [false]")
    parser.add_argument("--opt-strategy", choices=["sat-unsat", "unsat-sat", "binary"], default="sat-unsat",
                        help="Search algorithm for #amomaximize (each step is a new WASP run) [sat-unsat]")
    parser.add_argument("--progression", default="1", help="Step of the bound at step n: 1 (linear), 2 (+2), 2^n, n^2, ... [1]")
    parser.add_argument("--wasp", default=None, help="WASP executable [$WASP_EXE]")
    return parser.parse_args()


def run():
    a = parseArguments()
    sys.exit(runner(a.inputs, reason=a.reason, lazy=a.lazy, strategy=a.opt_strategy, progression=a.progression, wasp=a.wasp))


def optimized_run():
    """re-runs the module with -O (the parser's debug output off)"""
    os.execvp(sys.executable, [sys.executable, "-O", "-m", "amosum.amowasp", *sys.argv[1:]])


if __name__ == "__main__":
    run()
