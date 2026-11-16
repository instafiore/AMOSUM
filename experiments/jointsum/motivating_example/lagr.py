#!/usr/bin/env python3
"""Option 3: Lagrangian-aggregated knapsack constraint.
Reads a Knapsack .asp instance (object(X,W,V). ub(Cap,0). lb(Min,1).), computes the
root Dantzig bound with multiplicities 0..VMAX, takes lambda = ratio of the critical
item (num/den = VALUE_c/WEIGHT_c) and prints   lagr(Num,Den).   plus a comment with bounds.
The encoding then adds  sum_X V*(VALUE*Den - WEIGHT*Num) x >= Min*Den - Cap*Num."""
import re, sys
from fractions import Fraction
VMAX = 20
txt = open(sys.argv[1]).read()
objs = [tuple(map(int, m)) for m in re.findall(r'object\((\d+),(\d+),(\d+)\)', txt)]
cap = int(re.search(r'ub\((\d+),0\)', txt).group(1))
mn = int(re.search(r'lb\((\d+),1\)', txt).group(1))
items = sorted(objs, key=lambda o: Fraction(o[2], o[1]), reverse=True)
rest, lp, crit = cap, Fraction(0), None
for x, w, v in items:
    if VMAX * w <= rest:
        rest -= VMAX * w; lp += VMAX * v
    else:
        crit = (x, w, v); lp += Fraction(v * rest, w); break
if crit is None:  # everything fits: lambda = 0
    num, den = 0, 1
else:
    num, den = crit[2], crit[1]
lam = Fraction(num, den)
# Lagrangian value at lambda (= LP bound when lambda is the critical ratio)
lag = lam * cap + sum(max(0, VMAX * (v - lam * w)) for _, w, v in objs)
print(f"% cap={cap} min={mn} LP(Dantzig)={float(lp):.1f} L(lambda)={float(lag):.1f} critical={crit} -> {'UNSAT at root' if lag < mn else 'not refuted at root'}")
print(f"lagr({num},{den}).")
