#!/bin/bash
cd "$(dirname "$0")" || exit 1
SERVER_BENCHDIR=clown:/home/mazzotta/Experiments/instances/

includes=()
for i in GroupAssignmentOpt CombinatorialAuctionsOpt KnapsackOpt TspOpt GraphColouringOpt WOD; do
    includes+=(--include="$i/" --include="$i/encoding*.asp")
done

rsync -avz -P "${includes[@]}" --exclude="*" "$SERVER_BENCHDIR" .