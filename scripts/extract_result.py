#!/usr/bin/env python3
"""
Print the kind of problem solved in an amoclingo / amowasp output file and its
cost, as one tab-separated line:

    Optimization<TAB>V    if the output has an "Amomaximize Cost: V" line; V is
                          the value of the last (best) model. It is the value of
                          the #amomaximize: higher is better (clingo's
                          "Optimization:" would be its negation)
    Decision<TAB>0        otherwise

Usage:
    extract_result.py output.txt
    amoclingo ... | extract_result.py -

The output is expected in amoclingo's format (C++ and Python, also amowasp's):
with #amomaximize, every better model is printed on a line starting with
"Amomaximize Cost: V". An #amomaximize problem for which no model was found
(for example a run killed with "No solution found") has no such line, so it is
reported as Decision.
"""
import argparse
import re
import sys
from typing import Optional

# "Amomaximize Cost: 7 Wall Time: 0.001s Time ... AnswerSet 4° : { ... }"
AMOMAXIMIZE = re.compile(r"^Amomaximize Cost:\s*(?P<value>-?\d+)\b")


def last_value(text: str) -> Optional[str]:
    """Return the value of the last Amomaximize Cost line, or None if there is none."""
    value = None
    for line in text.splitlines():
        match = AMOMAXIMIZE.match(line.strip())
        if match:
            value = match.group("value")
    return value


def main() -> int:
    ap = argparse.ArgumentParser(description="Print Decision or Optimization and the #amomaximize value of an amoclingo output.")
    ap.add_argument("output", help="amoclingo/amowasp output file ('-' for stdin)")
    args = ap.parse_args()

    if args.output == "-":
        text = sys.stdin.read()
    else:
        with open(args.output) as fh:
            text = fh.read()
    value = last_value(text)
    print(f"Optimization\t{value}" if value is not None else "Decision\t0")
    return 0


if __name__ == "__main__":
    sys.exit(main())
