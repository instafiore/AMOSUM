#!/usr/bin/python3
import sys
import os
sys.path.append(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
import wasp
from typing import List
from utility import *
import re
from functional_propagator import *
import functional_propagator

'''
Propagator for ' <= UB ' constraint with At Most K constraint 

Invariants:
    In the aggregate set there are not two literals such that li = ~lj
'''

functional_propagator.propagator.prob_type = "AMK"
functional_propagator.propagator.ge = False