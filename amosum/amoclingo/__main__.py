"""The amoclingo command: amoclingo-core (C++, default) or -lg py (Python), same options.

    amoclingo encoding.asp instance.asp [--reason ...] [--lazy ...] [--opt-strategy ...] [--progression ...] [-lg py]

With C++ the ASP files are rewritten by the parser into temporary files and amoclingo-core is run on them (its output
and exit code are passed through, SIGINT/SIGTERM forwarded).
"""
import os
import signal
import subprocess
import sys
import tempfile
from typing import List

from amosum.amoclingo.py.options import core_arguments, parse_arguments
from amosum.amosumparser.rewrite import rewrite_all

CORE = os.path.join(os.path.dirname(os.path.abspath(__file__)), "cpp", "bin", "amoclingo-core")


def run_core(inputs: List[str], args: List[str]) -> int:
    with tempfile.TemporaryDirectory(prefix="amoclingo_") as tmp:
        rewritten = rewrite_all(inputs, tmp)
        # the options before the files: amoclingo-core's parser takes everything after the first file as a file
        proc = subprocess.Popen([CORE, *args, *rewritten])

        # SIGINT/SIGTERM are forwarded: amoclingo-core prints its best answer set and exits
        def forward(signum, frame):
            proc.send_signal(signum)
        signal.signal(signal.SIGINT, forward)
        signal.signal(signal.SIGTERM, forward)
        return proc.wait()          # the temporary directory and the rewritten files are removed here


def optimized_run():
    """re-runs the module with -O (the parser's debug output off)"""
    os.execvp(sys.executable, [sys.executable, "-O", "-m", "amosum.amoclingo", *sys.argv[1:]])


def run():
    known = parse_arguments()
    if known.lang == "cpp":
        sys.exit(run_core(known.inputs, core_arguments(known)))
    from amosum.amoclingo.py.main import main
    sys.exit(main(known))


if __name__ == "__main__":
    run()
