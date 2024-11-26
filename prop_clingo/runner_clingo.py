import json
import math
import subprocess
import time
from typing import Dict, List
import re
import sys
import os
from amosum import *
from clingo.symbol import Number
from clingo.control import Control

# adding root path
sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..')))

import ast
from prop_wasp.runner_wasp import RunnerWasp
from  utility import *
import utility
import settings
from prop_clingo.propagator_clingo import *
from preprocess import *

class RunnerClingo(RunnerWasp):
    '''
    This class is meant to run experiments on the AMO sum propagator(s) Clingo
    '''

    def __init__(self, parameters: Dict[str, str]) -> None:
        super().__init__(parameters)
    
    def run_instance(self, instance, encoding=None, group_type=True):
        
        # defining the lower bound(s)
        self.create_bound(instance=instance, ub=False)
    
        # defining the upper bound(s)
        self.create_bound(instance=instance, ub=True)


        # Setup encoding and instance file paths
        location_encoding = f"{self.location}/{encoding}.asp" if encoding else ""
        location_encoding = encoding if self.enc else  location_encoding
        location_encoding = f"{self.location}/{encoding}.asp" if self.enc and not self.exp else location_encoding
        location_instance = f"{self.location_instance}/{instance}.asp" if not self.exp else instance

        print(f"location_encoding {location_encoding}")
        print(f"location_instance {location_instance}")

        # Initialize the Clingo control object
        arguments = []
        models = f"--models={self.num_models}" if self.num_models != "" else ""
        seed = f"--seed={self.seed}" if self.seed != "" else ""
        arguments.append(models) if models != "" else ""
        arguments.append(seed) if seed != "" else ""
        self.ctl = Control(arguments=arguments)
        
        # Load the instance file
        self.ctl.load(location_instance)
        self.ctl.load(location_encoding) if encoding else ""
        self.ctl.load(self.str_weights) if self.str_weights != "" else ""
        self.ctl.load(self.str_lb) if self.str_lb != "" else ""
        self.ctl.load(self.str_ub) if self.str_ub != "" else ""
        
        # Ground the base part of the program
        self.ctl.ground([("base", [])])  # Ensure you ground with the correct subprogram
        grounded_program = ground_program(location_encoding, location_instance, self.str_weights, self.str_lb, self.str_ub)
        # print(f"grounded_program: {grounded_program}")
        
        preprocess_map = preprocess_ground_program(grounded_program)

        # return [], 0

        if group_type: 
            # initializing parameters 
            for amosum_id in preprocess_map["amosum_ids"]:
                self.registerPropagator(self.param.get("prop_type"), id=amosum_id)


        # Collect all models
        models = []

        def on_model(model):
            # Append the model's symbols to the list
            regex_query = self.get_regex_query_atom_answerset()
            model = set([str(atom) for atom in model.symbols(shown=True)])
            filtered_model = set()
            for atom in model:
                string = f" {atom}"
                result_match = re.search(regex_query, string)
                # print(f"atom str: {atom} regex_query: {regex_query} match: {result_match}")
                if result_match:
                    filtered_model.add(atom)
            # print(f"filtered_model: {filtered_model} model_str: {model} regex_query: {regex_query}")
            models.append(set(filtered_model))

            
        # Solve and get all models
        start_time = time.time()  
        handle : clingo.SolveHandle = self.ctl.solve(on_model=on_model, async_ = True)
        res = handle.wait(self.timeout_m * 60 if not self.exp else None)
        end_time = time.time()  # End time

        # stats = self.ctl.statistics
        # print("Statistics:", stats)
        
        wall_time = end_time - start_time
        wall_time = round(wall_time, 2) if res else "timeout"

        # restoring the instance.asp file
        self.comment_bound(instance=instance, ub=False, restore=True)
        self.comment_bound(instance=instance, ub=True, restore=True)
        
        return models, wall_time
    
    def registerPropagator(self, prop_type: str, id: str):
        match prop_type:
            case "ge_amo":
                ge = True
                prop_type = "AMO"
                from prop_wasp.ge_amo import propagate_phase
            case "le_amo":
                ge = False
                prop_type = "AMO"
                from prop_wasp.le_eo import propagate_phase
            case "ge_eo":
                ge = True
                prop_type = "EO"
                from prop_wasp.ge_eo import propagate_phase
            case _:
                assert False

        # Initialize and register the custom propagator
        param = self.param.copy()
        param["id"] = id
        propagator_clingo = PropagatorClingo(param, propagation_phase=propagate_phase, ge=ge, prop_type=prop_type)
        self.ctl.register_propagator(propagator_clingo)


    
