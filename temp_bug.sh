#!/usr/bin/bash

echo "" > out_temp 
echo "" > err_temp 
for i in {1..10}
do
    /home/s.fiorentino//AMOSUM/clingo_dir/run.py -problem kn \
    -enc /home/s.fiorentino/experiments/problems/IJCAI2024/Knapsack/encoding_with_group_ge_amo.asp  \
    -prop_type ge_amo -i /home/s.fiorentino/AMOSUM/tests/benchmarks/knapsack/instances_light/0018-knapsack-10-21684-297655-type4.asp \
    -id 1 -exp -seed 1 >> out_temp 2>> err_temp 
done