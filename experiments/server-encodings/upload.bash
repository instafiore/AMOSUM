#!/bin/bash
cd "$(dirname "$0")" || exit 1
SERVER_BENCHDIR=clown:/home/mazzotta/Experiments/instances/
BENCHS=(GroupAssignmentOpt CombinatorialAuctionsOpt KnapsackOpt TspOpt GraphColouringOpt WOD)

includes=()
for i in "${BENCHS[@]}"; do
    includes+=(--include="$i/" --include="$i/*")
done

# add -n to see what would be sent
rsync -avz -P "${includes[@]}" --exclude="*" "${BENCHS[@]}" "$SERVER_BENCHDIR"