#!/bin/bash

# Define the start and end of the range
# Initialize an empty array
numbers=()

# Set the initial value
value=100

# Loop until the value reaches 50000
while [ $value -le 20000 ]; do
    # Add the value to the array
    numbers+=("$value")
    
    # Decrement the value by 500
    ((value += 500))
done

# Iterate over the list and call run.py with each integer as a parameter
for number in "${numbers[@]}"; do
    ../../../clingo_dir/run.py -cc -problem kn -enc_type le_eo  -prop_type le_eo -nt 1 -l -write_res false  -ub "$number"
done