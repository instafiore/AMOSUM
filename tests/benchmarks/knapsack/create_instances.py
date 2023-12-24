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
std_dev_weight = 2000

# std dev of the value lb
std_dev_value = 20000

min_value = 1000
max_value = 2000
mean_object_value = (max_value + min_value)//2

min_weight= 100
max_weight = 200
mean_object_weight = (max_weight + min_weight)//2

instances = []
k=20

ind = 0 

start_n : int
end_n : int
instances_str : str

if light:
    start_n = 5
    end_n = 10
    instances_str = "instances_light"
else:
    start_n = 100
    end_n = 145
    instances_str = "instances"

for n in range(start_n,end_n+1,5):
    size = 1
    for i in range(size):

        # generating objects
        values = np.linspace(min_value, max_value, n)
        weights = np.linspace(min_weight , max_weight, n)

        values = np.around(values).astype(int)
        weights = np.around(weights).astype(int)

        # generating lbs
        for p in (0.60, 0.75, 0.90, 1.05, 1.20, 1.35, 1.50):
            
            mean_lb_weight =  n*k*mean_object_weight*p
            lbs_weight = np.random.normal(mean_lb_weight, std_dev_weight, 1)

            mean_lb_value =  n*k*mean_object_value*p
            lbs_values = np.random.normal(mean_lb_value, std_dev_value, 1)    

            # adding rows
            instance = [
                f"{ind:04}",
                n,
                int(lbs_weight[0]),
                int(lbs_values[0]),
                list(weights),
                list(values),
                p,
            ]
            instances.append(instance)
            ind+=1
            print(f"{instance[0]} {instance[2]} {p} {mean_lb_weight} {n*k*mean_object_weight}")




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
    with open(f'{instances_str}/{instance[0]}-knapsack-{instance[1]}-{instance[2]}-{instance[3]}.asp', 'w') as file:

        # writing objects 
        lb_0 = instance[2]
        lb_1 = instance[3]
        weights = instance[4]
        values  = instance[5]

        n = len(values)
        for i in range(n):
            file.write(f"object({i+1},{values[i]},{weights[i]}).\n")
        
        # writing bounds
        file.write(f"lb({-lb_0},0).\n")
        file.write(f"lb({lb_1},1).\n")
        

        


