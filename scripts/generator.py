#!/usr/bin/env python3
"""Generate one self-contained amoclingo-<config>.sh per configuration for CPP version.

The file name of each script is built from its configuration: KEY=VALUE pairs
joined by "-", in the order of the configuration, keys and values cut to 4
characters (after SHORT / SHORT_VALUE), with "-" inside a value replaced by "_",
for example
    amoclingo-reas=nomi-lazy=true-prog=nsq-opt=us.sh    (--progression n^2, --opt-strategy unsat-sat)

ijcai version, namely: amoclingo-base and amowasp-base are untouched
"""


TEMPLATE = r'''#!/bin/bash
#
# Configuration: @TAG@
#

trap ":" 24 15

# ---------------------------------------------------------------------------
# Inputs
# ---------------------------------------------------------------------------
FILENAME="$1"
if [[ -z "$FILENAME" ]]; then
  echo "usage: $0 INSTANCE_FILE" >&2
  exit 2
fi

INSTANCE_FOLD=$(dirname "$FILENAME")
instance_basename=$(basename "$FILENAME")

ENCODING="$INSTANCE_FOLD/encoding-amosum-@CONSTRAINT@.asp"
stats="stats/$instance_basename.stats"

if [[ $instance_basename == encoding* ]]; then
  echo "Ignoring $FILENAME"
  exit 0
fi

mkdir -p stats configuration

# ---------------------------------------------------------------------------
# Options: only non-empty variables are passed
# ---------------------------------------------------------------------------
OPTS=()
add(){ [[ -n "$2" ]] && OPTS+=("--$1=$2"); }

@ADDS@


# static parameters
add statsfile "$stats"

CMD=(amoclingo "${OPTS[@]}" "$ENCODING" "$FILENAME")

options_file=configuration/options
help_file=configuration/help

[[ ! -s $options_file ]] && printf '%q\n' "${CMD[@]}" > $options_file
[[ ! -s $help_file ]]    && amoclingo -h > $help_file


printf 'executing'
printf ' %q' "${CMD[@]}"
printf '\n'

"${CMD[@]}"
'''


import os, shlex, stat, sys
from pathlib import Path

OUT = sys.argv[1] if len(sys.argv) > 1 else "amosum-scripts"

path_out = Path(OUT)

path_out.mkdir(exist_ok=True)

for old_file in path_out.iterdir():
    old_file.unlink() 

# {variable: value}; variables not given are not passed to metaprop.
CONFIGS = [
    # AMOSUM
    dict(reason="nomin", lazy="true"),
    dict(reason="nomin", lazy="false"),
    dict(reason="minfly", lazy="false"),
    dict(reason="minfly", lazy="true"),
    # AMOMAXIMIZE
    dict(reason="ijcai", lazy="false", progression="1", opt_strategy="sat-unsat"),
    dict(reason="minfly", lazy="true", progression="1", opt_strategy="sat-unsat"),
    dict(reason="nomin", lazy="true", progression="1", opt_strategy="sat-unsat"),
]

# AMOMAXIMIZE: the search strategies with the best configuration above (nomin, lazy: the most wins and the best
# mean score over the 6 optimization benchmarks); binary search does not use --progression
BEST = dict(reason="nomin", lazy="true")
BASE = dict(reason="ijcai", lazy="false")

for strategy in ["sat-unsat", "unsat-sat"]:
    for progression in ["n", "n^2", "2^n"]:
        CONFIGS.append(dict(**BEST, progression=progression, opt_strategy=strategy))
        CONFIGS.append(dict(**BASE, progression=progression, opt_strategy=strategy))
        
CONFIGS.append(dict(**BEST, opt_strategy="binary"))

COMMON = {"lang": "cpp", "verbose": "progression-last-model"}

SHORT = {
    "opt_strategy": "opt",
}

# short values for the file names; "^" would be a regex anchor in the batch's solver regex
SHORT_VALUE = {
    "sat-unsat": "su",
    "unsat-sat": "us",
    "binary": "bin",
    "n^2": "nsq",
    "2^n": "exp",
}

for conf in CONFIGS:
    conf.update({**COMMON})

MAX_LENGTH_KEY=4
MAX_LENGTH_VALUE=4

constraints = ["amo", "eo"]
solvers = ["clingo"]
files = []
for solver in solvers:
    for constraint in constraints:
        for conf in CONFIGS:
            solver_script = TEMPLATE[::]
            adds = "\n".join(f"add {str(k).replace("_","-")} {shlex.quote(str(v))}" for k, v in conf.items())
            prefix = f"{constraint}{solver}"
            suffix = "-".join(f"{str(SHORT.get(k,k))[:MAX_LENGTH_KEY]}={str(SHORT_VALUE.get(v,v)).replace("-","_")[:MAX_LENGTH_VALUE]}" for k, v in conf.items() if k not in COMMON)
            solver_name = "-".join([prefix, suffix])
            solver_name = f"{solver_name}.sh"

            solver_script = solver_script\
                .replace("@TAG@", " ".join(f"--{str(k).replace('_','-')}={v}" for k, v in conf.items()))\
                .replace("@CONSTRAINT@", constraint)\
                .replace("@ADDS@", adds)
            # print(solver_script)
            filePath = path_out / solver_name
            filePath.write_text(solver_script)
            print(f"writing {solver_name}")
            files.append(solver_name)

print(f"Wrote {len(files)} into {OUT}")
