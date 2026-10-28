"""amowasp (mirrors amoclingo/py/main.py): the options, the Runner, the exit code."""
import argparse
import os
import shutil
import sys

from amosum.amowasp.py.runner import Runner


def main(known: argparse.Namespace) -> int:
    wasp = known.wasp or os.environ.get("WASP_EXE") or shutil.which("wasp")
    if not wasp:
        print("WASP not found: set WASP_EXE or put wasp on the PATH", file=sys.stderr)
        return 1
    print("amowasp version 3.0", flush=True)
    result = Runner().run(known.inputs, wasp, reason=known.reason, lazy=known.lazy, strategy=known.opt_strategy,
                          progression=known.progression)
    return result.exit_code()
