"""The amowasp command: amosum on WASP (the WASP executable: --wasp, or the environment variable WASP_EXE).

    amowasp encoding.asp instance.asp [--reason ...] [--lazy ...] [--opt-strategy ...] [--progression ...]

Same options (those that change the search), output and exit codes as amoclingo.
"""
import os
import sys

from amosum.amowasp.py.main import main
from amosum.amowasp.py.options import parse_arguments


def run():
    sys.exit(main(parse_arguments()))


def optimized_run():
    """re-runs the module with -O (the parser's debug output off)"""
    os.execvp(sys.executable, [sys.executable, "-O", "-m", "amosum.amowasp", *sys.argv[1:]])


if __name__ == "__main__":
    run()
