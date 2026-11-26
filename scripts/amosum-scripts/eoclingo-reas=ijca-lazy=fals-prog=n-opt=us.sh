#!/bin/bash
#
# Configuration: --reason=ijcai --lazy=false --progression=n --opt-strategy=unsat-sat --lang=cpp --verbose=progression-last-model
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

ENCODING="$INSTANCE_FOLD/encoding-amosum-eo.asp"
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

add reason ijcai
add lazy false
add progression n
add opt-strategy unsat-sat
add lang cpp
add verbose progression-last-model


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
