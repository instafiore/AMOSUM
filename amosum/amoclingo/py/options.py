"""The options of amoclingo (mirrors amoclingo/cpp/options.h and options.cpp): amoclingo-core's options plus --lang."""
import argparse
import re
from typing import List

LOG_LEVELS = ["ALWAYS", "UNKNOWN", "DEBUG_3", "DEBUG_2", "DEBUG_1", "DEBUG", "CRITICAL", "ERROR",
              "CLINGO_ERROR", "CLINGO_WARNING", "WARNING", "INFO_2", "INFO_1", "INFO"]

# amoclingo-core's options with a value: (name, argparse arguments); forwarded as --name value only when given
AMOCLINGO_OPTIONS = [
    ("reason", dict(choices=["nomin", "ijcai", "min", "minfly", "cmin"],
                    help="Reasons of the >= derivations [nomin]")),
    ("lazy", dict(help="When to propagate: false, true, hybrid, or a float in [0,1] [false]")),
    ("opt-strategy", dict(choices=["sat-unsat", "unsat-sat", "binary"], help="Search algorithm for #amomaximize: upwards, downwards or binary search [sat-unsat]")),
    ("progression", dict(help="Step of the bound at step n: 1 (linear), 2 (+2), 2^n, n^2, ... [1]")),
    ("log", dict(choices=LOG_LEVELS, help="Log level (debug builds) [INFO]")),
    ("log-file-name", dict(help="Path for log file (debug builds) [fiore.log]")),
    ("statsfile", dict(help="Path to stats file [fiore.stats]")),
    ("seed", dict(help="Seed [42]")),
    ("verbose", dict(choices=["1", "2"], help="1: cost and time per answer set, 2: cost, time and answer set [1]")),
]


def lazy_value(value: str) -> str:
    """the regex of amoclingo-core's LazyValue"""
    if not re.fullmatch(r"false|true|hybrid|0(\.\d+)?|1(\.0+)?", value):
        raise argparse.ArgumentTypeError(f"{value}: expected false, true, hybrid, or a float in [0,1]")
    return value


def parse_arguments(argv=None) -> argparse.Namespace:
    # allow_abbrev=False: no prefixes of options (otherwise -l, the --lazy of -lg py, would be taken as -lg)
    parser = argparse.ArgumentParser(prog="amoclingo", description="Clingo with #amosum, #eosum and #amomaximize", allow_abbrev=False)
    parser.add_argument("inputs", nargs="*", metavar="asp-input", help="ASP files (encoding, instance, ...)")
    parser.add_argument("-lg", "--lang", choices=["cpp", "py"], default="cpp",
                        help="Propagator: C++ (amoclingo-core) or Python (clingo's Python API), same options [cpp]")
    for name, kwargs in AMOCLINGO_OPTIONS:
        if name == "lazy": kwargs = dict(kwargs, type=lazy_value)
        parser.add_argument(f"--{name}", default=None, **kwargs)
    parser.add_argument("--TimestampLogFile", action="store_true", help="Set the timestamp to the log file")
    known = parser.parse_args(argv)
    if not known.inputs:
        parser.error("at least one ASP input is required")
    return known


def core_arguments(known: argparse.Namespace) -> List[str]:
    """the options given by the user, in amoclingo-core's syntax (--lang and the ASP files are not among them)"""
    args = []
    for name, _ in AMOCLINGO_OPTIONS:
        value = getattr(known, name.replace("-", "_"))
        if value is not None: args += [f"--{name}", value]
    if known.TimestampLogFile: args.append("--TimestampLogFile")
    return args
