#!/bin/bash
cd "$(dirname "$0")" || exit 1
SERVER_BENCHDIR=clown:/home/mazzotta/Experiments/instances/

# amosum (decision) benchmarks of the paper, with their names on the server (as in results/AIJ-2)
AMOSUM_BENCHS=(CombinatorialAuction GraphColouring GroupAssignment Knapsack Nurse TspDecisional)
# amomaximize (optimization) benchmarks
AMOMAXIMIZE_BENCHS=(GroupAssignmentOpt CombinatorialAuctionsOpt KnapsackOpt TspOpt GraphColouringOpt WOD)

includes=()
for i in "$AMOSUM_BENCHS{@}" "$AMOMAXIMIZE_BENCHS{@}"; do
    includes+=(--include="$i/" --include="$i/encoding*.asp")
done

rsync -avz -P "${includes[@]}" --exclude="*" "$SERVER_BENCHDIR" .