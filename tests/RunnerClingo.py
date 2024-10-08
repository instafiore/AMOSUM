import json
import subprocess
from typing import Dict, List
import re
import sys
import os
import clingo
from propagator import *
from clingo.symbol import Number
from clingo.control import Control

# adding root path
sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..')))
import ast
from tests.RunnerWasp import RunnerWasp
from  utility import *
import utility
import settings
from clingo_dir.ge_amo import *
from wasp_dir.ge_amo import propagate_phase

class RunnerClingo(RunnerWasp):
    '''
    This class is meant to run experiments on the AMO sum propagator(s) Clingo
    '''

    def __init__(self, parameters: Dict[str, str]) -> None:
        super().__init__(parameters)
    
    def run_instance(self, instance, encoding=None, group_type=True):
        
        location_encoding = f"{self.location}/{encoding}.asp" if encoding else ""
        location_instance = f"{self.location_instance}/{instance}.asp"

        ctl = Control()
        ctl.load(location_instance)
        ctl.ground()
        
        ge_amo_propagator = GeAmo(self.param, propagation_phase=propagate_phase)
        
        # Registering propagator
        ctl.register_propagator(ge_amo_propagator)
        
        print(ctl.solve(on_model=print))

        # Default returning no answersets and time 0
        return (None, 0)

    
