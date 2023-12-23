#!/bin/bash

# Initialize an empty array
my_array=()

s=$1
e=$2
i=$3
l=$4

for ((lb = $s; lb >= $e; lb -= 5)); do
  my_array+=("$lb")
done

T=$(date "+%Y-%m-%d.%H.%M")
echo "$T"

for lb in "${my_array[@]}"; do
  make run_test p=graph_colouring i="$i" lb="$lb" l="$l" t="$T"
done

