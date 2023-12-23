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

# std dev of the weight lb
std_dev_weight = 20000

# std dev of the value lb
std_dev_value = 2000

min_value = 100
max_value = 200

min_weight= 1000
max_weight = 2000

instances = []
k=20

ind = 0 
for n in range(100,145+1,5):
    size = 2
    for i in range(size):

        # generating objects
        values = np.linspace(min_value, max_value, n)
        weights = np.linspace(min_weight , max_weight, n)

        values = np.around(values).astype(int)
        weights = np.around(weights).astype(int)

        # generating lbs
        for p in (0.15, 0.30, 0.45, 0.60, 0.75):
            
            mean_object_weight = (max_weight + min_weight)//2
            mean_lb_weight =  n*k*mean_object_weight*p
            lbs_weight = np.random.normal(mean_lb_weight, std_dev_weight, 1)

            mean_object_value = (max_value + min_value)//2
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
            print(f"{instance[2]} {p} {mean_lb_weight}")


