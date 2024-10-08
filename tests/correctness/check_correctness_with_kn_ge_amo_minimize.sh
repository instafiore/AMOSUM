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

echo "C_MIN"
for number in "${numbers[@]}"; do
    ../../wasp_dir/run.py -problem kn -enc_type ge_amo -prop_type ge_amo  -nt 1 -l -id 1 -write_res false -min_r c_min -lb "$number"
done

echo "MIN"
for number in "${numbers[@]}"; do
    ../../wasp_dir/run.py -problem kn -enc_type ge_amo  -prop_type ge_amo -nt 1 -l -id 1 -write_res false -min_r min -lb "$number"
done