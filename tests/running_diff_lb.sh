#!/bin/bash

# Initialize an empty array
my_array=()

# Use a for loop to populate the array
for ((i = 250; i >= 200; i -= 2)); do
  my_array+=("$i")
done

T=$(date "+%Y-%m-%d.%H.%M")
echo "$T"

# Print the array elements
cd ..
for lb in "${my_array[@]}"; do
  make run_test p=graph_colouring i=0001-graph_colouring-125-0 lb="$lb" l=1 t="$T"
done

