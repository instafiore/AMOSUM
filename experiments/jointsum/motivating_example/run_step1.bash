#!/usr/bin/env bash
# Option-3 check: plain clingo + Lagrangian-aggregated implied constraint.
# usage: run_step1.bash <instance.asp>... (TIMEOUT env var, default 60s)
here=$(cd "$(dirname "$0")" && pwd)
for inst in "$@"; do
  lam=$(mktemp); python3 "$here/lagr.py" "$inst" > "$lam"
  printf '%s\t%s\t' "$(basename "$inst")" "$(head -1 "$lam")"
  clingo "$here/encoding-plain-amo-lagr.asp" "$inst" "$lam" -q --stats --time-limit="${TIMEOUT:-60}" 2>/dev/null \
    | grep -E '^(SATISFIABLE|UNSATISFIABLE|UNKNOWN|Time|Conflicts)' | tr -s ' \n' ' '; echo
  rm -f "$lam"
done
