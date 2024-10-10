#!/bin/bash

# Define the start and end of the range
# Initialize an empty array
numbers=()

# Set the initial value
value=30000

# Loop until the value reaches 50000
while [ $value -ge 0 ]; do
    # Add the value to the array
    numbers+=("$value")
    
    # Decrement the value by 500
    ((value -= 500))
done

# Iterate over the list and call run.py with each integer as a parameter
for number in "${numbers[@]}"; do
    ../../../clingo_dir/run.py -cc -problem kn -enc_type ge_eo  -prop_type ge_eo -nt 1 -l -id 1 -write_res false  -lb "$number"
done