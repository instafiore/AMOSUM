#!/home/mazzotta/Experiments/solvers/bin_fiorentino/amosum_env/bin/python
import argparse
import os
import re
import sys
from typing import List

from amosum.amoclingo.propagator_clingo_c.runner_clingo import RunnerClingoCpp

'''
This is the entry file to run the AMO SUM propagator(s) CLINGO
'''

LOG_LEVELS = ["ALWAYS", "UNKNOWN", "DEBUG_3", "DEBUG_2", "DEBUG_1", "DEBUG", "CRITICAL", "ERROR",
              "CLINGO_ERROR", "CLINGO_WARNING", "WARNING", "INFO_2", "INFO_1", "INFO"]

# amoclingo-core's options with a value: (name, argparse arguments); forwarded as --name value only when given
AMOCLINGO_OPTIONS = [
    ("reason", dict(choices=["nomin", "ijcai", "min", "minfly", "cmin"],
                    help="Reasons of the >= derivations [nomin]")),
    ("lazy", dict(help="When to propagate: false, true, hybrid, or a float in [0,1] [false]")),
    ("opt-strategy", dict(choices=["sat-unsat", "unsat-sat"], help="Search algorithm for #amomaximize: upwards or downwards [sat-unsat]")),
    ("progression", dict(help="Step of the bound at step n: 1 (linear), 2 (+2), 2^n, n^2, ... [1]")),
    ("log", dict(choices=LOG_LEVELS, help="Log level (debug builds) [INFO]")),
    ("log-file-name", dict(help="Path for log file (debug builds) [fiore.log]")),
    ("statsfile", dict(help="Path to stats file [fiore.stats]")),
    ("seed", dict(help="Seed [42]")),
    ("verbose", dict(choices=["1", "2"], help="1: cost and time per answer set, 2: cost, time and answer set [1]")),
]


def lazyValue(value: str) -> str:
    # same regex as amoclingo-core's LazyValue
    if not re.fullmatch(r"false|true|hybrid|0(\.\d+)?|1(\.0+)?", value):
        raise argparse.ArgumentTypeError(f"{value}: expected false, true, hybrid, or a float in [0,1]")
    return value


def parseArguments() -> argparse.Namespace:
    parser = argparse.ArgumentParser(prog="amoclingo", description="Clingo with #amosum, #eosum and #amomaximize")
    parser.add_argument("inputs", nargs="*", metavar="asp-input", help="ASP files (encoding, instance, ...)")
    parser.add_argument("-lg", "--lang", choices=["cpp", "py"], default="cpp", help="Propagator: C++ or Python [cpp]")
    for name, kwargs in AMOCLINGO_OPTIONS:
        if name == "lazy": kwargs = dict(kwargs, type=lazyValue)
        parser.add_argument(f"--{name}", default=None, **kwargs)
    parser.add_argument("--TimestampLogFile", action="store_true", help="Set the timestamp to the log file")

    # parse_known_args: with -lg py the old options (-e, -i, ...) are read by parse_args
    known, rest = parser.parse_known_args()
    if known.lang == "cpp" and rest:
        parser.error(f"unrecognized arguments: {' '.join(rest)}")
    if known.lang == "cpp" and not known.inputs:
        parser.error("at least one ASP input is required")
    return known


def amoclingoArguments(known: argparse.Namespace) -> List[str]:
    # the options given by the user, in amoclingo-core's syntax (--lang and the ASP files are not among them)
    args = []
    for name, _ in AMOCLINGO_OPTIONS:
        value = getattr(known, name.replace("-", "_"))
        if value is not None: args += [f"--{name}", value]
    if known.TimestampLogFile: args.append("--TimestampLogFile")
    return args


def optimized_run():
    """
    Wrapper that re-runs the module with -O optimization flag.
    """

    os.execvp(sys.executable, [sys.executable, "-O", "-m", "amosum.amoclingo", *sys.argv[1:]])


def runPython():
    # RunnerClingoPython imports utility and settings as top-level modules: amosum/ must be on the path
    sys.path.append(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
    from amosum.amoclingo.propagator_clingo_py.runner_clingo import RunnerClingoPython
    from amosum.utility import parse_args, ERROR_CODE

    params = parse_args()
    print(f"Parameters: {params}")
    results = RunnerClingoPython(parameters=params).run()
    exit(results[0].exitCode if results else ERROR_CODE)


def run():
    known = parseArguments()
    if known.lang == "cpp":
        exit(RunnerClingoCpp(known.inputs, amoclingoArguments(known)).run())   # amoclingo-core's exit code
    runPython()

if __name__ == '__main__':
    run()