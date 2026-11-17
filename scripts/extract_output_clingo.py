#!/usr/bin/env python3
"""
Print the kind of problem solved in a clingo output in the competition format
(clingo --outf=1) and its cost, as one tab-separated line, as extract_result.py
does for amoclingo / amowasp:

    Optimization<TAB>V    if the output has a "COST" line; V is the cost of the
                          last (best) model. By default it is negated, as the
                          value of a #maximize (clingo minimizes, so a #maximize
                          of value 9 is printed as COST -9): the same number as
                          amoclingo's "Amomaximize Cost". --minimize keeps it
    Decision<TAB>0        otherwise

Usage:
    extract_result_clingo.py output.txt
    clingo enc.asp inst.asp --outf=1 | extract_result_clingo.py -
    extract_result_clingo.py --minimize output.txt          a #minimize program

The competition format, without the "%" comment lines:

    ANSWER
    a(2). a(3). a(4).
    COST -9                   (one number per priority level, highest first: "COST 3 -9")
    OPTIMUM                   (or nothing if not proven; INCONSISTENT and no ANSWER if unsatisfiable)

Only the last (best) model is printed, also when the run is stopped (--time-limit,
SIGINT); if there are several COST lines the last one is used. With several
priority levels the highest one is used. A run killed
before any model (no COST line) is reported as Decision, as in extract_result.py.
"""
import argparse
import re
import sys
from typing import Optional

# "COST -9" or "COST 3 -9" (several levels, highest first); "-9@1" also accepted
COST = re.compile(r"^COST\s+(?P<value>-?\d+)(?:@-?\d+)?")


def last_cost(text: str) -> Optional[int]:
    """Return the cost (highest priority level) of the last COST line, or None if there is none."""
    value = None
    for line in text.splitlines():
        match = COST.match(line.strip())
        if match:
            value = int(match.group("value"))
    return value


def main() -> int:
    ap = argparse.ArgumentParser(description="Print Decision or Optimization and the cost of a clingo --outf=1 output.")
    ap.add_argument("output", help="clingo --outf=1 output file ('-' for stdin)")
    ap.add_argument("--minimize", action="store_true",
                    help="print clingo's cost as it is (a #minimize); default: negated, the value of a #maximize")
    args = ap.parse_args()

    if args.output == "-":
        text = sys.stdin.read()
    else:
        with open(args.output) as fh:
            text = fh.read()
    cost = last_cost(text)
    if cost is None:
        print("Decision\t0")
    else:
        print(f"Optimization\t{cost if args.minimize else -cost}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
