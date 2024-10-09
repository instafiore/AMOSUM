import json
import subprocess
import time
from typing import Dict, List
import re
import sys
import os
import clingo
from propagator_wasp import *
from clingo.symbol import Number
from clingo.control import Control

# adding root path
sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..')))
import ast
from tests.RunnerWasp import RunnerWasp
from  utility import *
import utility
import settings
from clingo_dir.propagator_clingo import *
from wasp_dir.ge_amo import propagate_phase

class RunnerClingo(RunnerWasp):
    '''
    This class is meant to run experiments on the AMO sum propagator(s) Clingo
    '''

    def __init__(self, parameters: Dict[str, str]) -> None:
        super().__init__(parameters)
    
    def run_instance(self, instance, encoding=None, group_type=True):
        
        # Setup encoding and instance file paths
        location_encoding = f"{self.location}/{encoding}.asp" if encoding else ""
        location_instance = f"{self.location_instance}/{instance}.asp"

        # Initialize the Clingo control object
        ctl = Control(arguments=["--models=0"])
        
        # Load the instance file
        ctl.load(location_instance)
        
        # Ground the base part of the program
        ctl.ground([("base", [])])  # Ensure you ground with the correct subprogram

        # Initialize and register the custom propagator
        ge_amo_propagator = PropagatorClingo(self.param, propagation_phase=propagate_phase, ge=True, prop_type="AMO")
        ctl.register_propagator(ge_amo_propagator)

        # Collect all models
        models = []

        def on_model(model):
            # Append the model's symbols to the list
            models.append(model.symbols(shown=True))

        # Solve and get all models
        start_time = time.time()  
        ctl.solve(on_model=on_model)
        end_time = time.time()  # End time
        
        wall_time = end_time - start_time

        # Return all models and a dummy time value 
        return models, wall_time


    
