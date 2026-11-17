#!/bin/bash
# by F. Ricca

if [ $# -lt 2 ]; then
    echo command result-directory outputfile 
    exit
fi

# Absolute path to this script
scriptpath=$(readlink -e "$0")
# Absolute path this script is in
DIRNAME=$(dirname "$scriptpath")

RESULTS=$1
OUT=$2
SOLVER=$3

echo $RESULTS 1>&2

rm .extractor.cumulative.err
for f in $RESULTS/*.err; do
   cat $f >> .extractor.cumulative.err
done
wc -l .extractor.cumulative.err



s=${SOLVER%.bash}
if [[ $s == clingo* ]]; then 
   parser=/home/mazzotta/Experiments/solvers/bin_fiorentino/AMOSUM/scripts/extract_output_clingo.py
elif [[ $s =~ (amo|eo)clingo ]]; then
   parser=/home/mazzotta/Experiments/solvers/bin_fiorentino/AMOSUM/scripts/extract_output_amosum.py
else
   parser=""                                   # no parser: the last column stays empty
fi

shopt -s nullglob                              # no .out files: the loop does not run at all
for f in "$RESULTS"/*.out; do
   echo "$f"

   line_parser=""
   if [[ -n $parser ]]; then
      line_parser=$(python "$parser" "$f")
   fi

   printf '%s\t' "$f" >> "$OUT"
   "$DIRNAME"/scripts/filenameToFeature.pl "$f" >> "$OUT"
   "$DIRNAME"/scripts/canonizeOut.pl < "$f" >> "$OUT"
   prl=${f%.out}.pyrl
   if [[ -f $prl ]]; then
      "$DIRNAME"/scripts/canonizePrl.pl < "$prl" >> "$OUT"
   else
      echo "missing $prl" >&2
   fi
   if [[ -n $line_parser ]]; then
      printf '\t%s\n' "$line_parser" >> "$OUT"
   else
      printf '\n' >> "$OUT"
   fi
done

