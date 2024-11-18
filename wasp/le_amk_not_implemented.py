#!/usr/bin/python3
import sys
import os
sys.path.append(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
import wasp
from typing import List
from utility import *
import re
from wasp.propagator_wasp import *
import wasp.propagator_wasp as propagator_wasp

'''
Propagator for ' <= UB ' constraint with At Most K constraint 

Invariants:
    In the aggregate set there are not two literals such that li = ~lj
'''

propagator_wasp.propagator.prob_type = "AMK"
propagator_wasp.propagator.ge = False