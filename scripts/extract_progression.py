#!/usr/bin/env python3
"""
Print the progression of an #amomaximize run, the (time, cost) pairs of its
improving models, as a Python list: one pair per model, in the order found.

    [(0.001307, 0), (0.002011, 4), (0.002654, 6), (0.003102, 7)]

time is the wall time (seconds from the start of the run) printed with the
model, cost the value of its #amomaximize (higher is better).

Usage:
    extract_progression.py output.txt                 one list
    extract_progression.py out1.txt out2.txt ...      a dict {file: list}
    amoclingo ... | extract_progression.py -

The output is expected in amoclingo's format (C++ and Python, also amowasp's).
The pairs come from the lines with both "Amomaximize Cost: V" and
"Wall Time: Ts", printed for every improving model with --verbose each-model
(the default), cost-only and progression-last-model:

    Amomaximize Cost: 7 Wall Time: 0.003102s Time 0.0001 Unkown AnswerSet 4° : { ... }
    Amomaximize Cost: 7 Wall Time: 0.003102s

The last line of a run carries no wall time and repeats the best model, so it
adds no pair. With --verbose last-model no model is printed during the search
and the list is empty (a warning goes to stderr).

From Python (e.g. for a plot):
    from extract_progression import read_progression
    times, costs = zip(*read_progression("output.txt"))
"""
import argparse
import re
import sys
from typing import Dict, List, Tuple

# "Amomaximize Cost: 7 Wall Time: 0.003102s ..."
POINT = re.compile(r"^Amomaximize Cost:\s*(?P<cost>-?\d+)\s+Wall Time:\s*(?P<time>[0-9.eE+-]+)s\b")


def parse_progression(text: str) -> List[Tuple[float, int]]:
    """the (wall time, #amomaximize value) of every improving model in an output, in the order printed"""
    points = []
    for line in text.splitlines():
        match = POINT.match(line.strip())
        if match:
            points.append((float(match.group("time")), int(match.group("cost"))))
    return points


def read_progression(path: str) -> List[Tuple[float, int]]:
    """parse_progression of a file ('-' for stdin)"""
    if path == "-":
        return parse_progression(sys.stdin.read())
    with open(path) as f:
        return parse_progression(f.read())


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__.split("\n\n")[0].strip())
    ap.add_argument("outputs", nargs="+", help="amoclingo/amowasp output files ('-' for stdin)")
    args = ap.parse_args()

    progressions: Dict[str, List[Tuple[float, int]]] = {}
    for path in args.outputs:
        progressions[path] = read_progression(path)
        if not progressions[path]:
            print(f"{path}: no 'Amomaximize Cost: V Wall Time: Ts' line (no #amomaximize, no model found, "
                  f"or --verbose last-model)", file=sys.stderr)

    print(progressions[args.outputs[0]] if len(args.outputs) == 1 else progressions)
    return 0


if __name__ == "__main__":
    sys.exit(main())
