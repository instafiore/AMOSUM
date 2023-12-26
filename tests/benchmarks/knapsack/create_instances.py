#!/usr/bin/python3
import sys
import os
import re
sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '../../../')))
import settings
import subprocess
from scipy.stats import norm
import numpy as np
import math

light = sys.argv[1] if len(sys.argv) > 1 else "normal"
if light == "light":
    light = True
elif light == "normal":
    light = False
else:
    raise Exception("invalid parameter")


# std dev of the weight lb
std_dev_weight = 1000

min_weight= 100
max_weight = 200
mean_object_weight = (max_weight + min_weight)//2

# std dev of the value lb
std_dev_value = 5000

min_value = 1000
max_value = 2000
mean_object_value = (max_value + min_value)//2

instances = []
k=20

ind = 0 

start_n : int
instances_str : str
step = 5
nips = 10

if light:
    start_n = 5
    instances_str = "instances_light"
    N = 20
else:
    start_n = 10
    instances_str = "instances"
    N = 100

end_n : int = step * ( N // nips - 1) + start_n

for n in range(start_n,end_n+1,5):

    # generating objects
    values = np.random.uniform(min_value, max_value, n)
    weights = np.random.uniform(min_weight , max_weight, n)

    values = np.around(values).astype(int)
    weights = np.around(weights).astype(int)

    C1_w =  n*k*mean_object_weight
    
    C1_v =  n*k*mean_object_value
    C2_v =  n * mean_object_value * (k * (k+1)) / 2

    size_1 = int(nips * 0.6)
    size_2 = size_1 + int(nips * 0.2)
    size_3 = size_2 + int(nips * 0.2)

    for i in range(nips):

        type : str
        lbs_value : int

        lbs_weight = int(np.random.normal(C1_w * 0.7, std_dev_weight))
        

        if i < size_1:

            if i < size_1 * 0.5:
                type = "type1"
                C = C1_v
            else:
                type = "type2"
                C = C2_v
        
            lbs_value = int(np.random.uniform(0, C))

        elif i < size_2:

            type = "type3" 
            lbs_value = int(np.random.uniform(C1_v*0.10, C1_v*1.1))

        else:

            type = "type4" 
            lbs_value = int(np.random.normal(C1_v, std_dev_value))

        # adding rows
        instance = [
            f"{ind:04}",
            n,
            lbs_weight,
            lbs_value,
            list(weights),
            list(values),
            type,
        ]
        instances.append(instance)
        ind+=1
        print(f"{instance[0]} {instance[1]} {instance[2]} {instance[3]} {instance[6]}")
        
    


        
        
    




'''
object(1,100,1000).
object(2,200,4000).
object(3,120,2500).
object(4,100,1000).
object(5,200,4000).
'''

subprocess.run(f"rm -rf {instances_str}/*", shell=True)

# creating instances
for instance in instances:
    with open(f'{instances_str}/{instance[0]}-knapsack-{instance[1]}-{instance[2]}-{instance[3]}-{instance[6]}.asp', 'w') as file:

        # writing objects 
        lb_0 = instance[2]
        lb_1 = instance[3]
        weights = instance[4]
        values  = instance[5]

        if(lb_0 < 0 or lb_1 < 0):
            print(f"INVALID VALUES ! -> {instance[0]} {instance[1]} \
                [{instance[2]}, {instance[7]}] \
                [{instance[3]}, {instance[8]}] {instance[6]}")

        n = len(values)
        for i in range(n):
            file.write(f"object({i+1},{weights[i]},{values[i]}).\n")
        
        # writing bounds
        file.write(f"lb({lb_0},0).\n")
        file.write(f"lb({lb_1},1).\n")
        

        


