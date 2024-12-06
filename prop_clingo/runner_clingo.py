from io import StringIO
import json
import math
import subprocess
import time
from typing import Dict, List
import re
import sys
import os
import AmoSumParser
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
from datetime import datetime

class RunnerClingo(RunnerWasp):
    '''
    This class is meant to run experiments on the AMO sum propagator(s) Clingo
    '''

    def __init__(self, parameters: Dict[str, str]) -> None:
        super().__init__(parameters)
    
    def run_instance(self, instance, encoding=None):
        

        # Get the current date and time
        now = datetime.now()

        # Format it as a string
        date_string = now.strftime("%Y-%m-%d-%H-%M")
        
        # defining the lower bound(s)
        self.create_bound(instance=instance, ub=False)
    
        # defining the upper bound(s)
        self.create_bound(instance=instance, ub=True)


        # Setup encoding and instance file paths
        location_encoding = f"{self.location}/{encoding}.asp" if encoding else ""
        location_encoding = encoding if self.enc else  location_encoding
        location_encoding = f"{self.location}/{encoding}.asp" if self.enc and not self.exp else location_encoding
        location_instance = f"{self.location_instance}/{instance}.asp" if not self.exp else instance

        print(f"encoding: {location_encoding}")
        print(f"instance: {location_instance}")

        # Initialize the Clingo control object
        arguments = []
        models = f"--models={self.num_models}" if self.num_models != "" else ""
        seed = f"--seed={self.seed}" if self.seed != "" else ""
        arguments.append(models) if models != "" else ""
        arguments.append(seed) if seed != "" else ""
        arguments.append("--stats=2")
        # arguments.append("--configuration=tweety")
    
        self.ctl = Control(arguments=arguments)
        self.propagators: List[PropagatorClingo] = []
        # Load the instance file
        self.ctl.load(location_instance)
        self.ctl.load(self.str_weights) if self.str_weights != "" else ""
        self.ctl.load(self.str_lb) if self.str_lb != "" else ""
        self.ctl.load(self.str_ub) if self.str_ub != "" else ""

        # Rewrinting without #amosum construct

        hidden_location_encoding= self.rewrite_file_without_amosum(location_encoding)
        # print(f"grounded file:\n {cat(hidden_location_encoding)}")
        grounded_program = ground_program(hidden_location_encoding, location_instance, self.str_weights, self.str_lb, self.str_ub)
        # print(f"grounded_program: {grounded_program}")

        preprocess_map = preprocess_ground_program(grounded_program)
        self.ctl.load(hidden_location_encoding) if encoding else ""
        self.ctl.ground([("base", [])])  # Ensure you ground with the correct subprogram
        # print(f"preprocess_map: {preprocess_map}")

        
        for amosum in preprocess_map["amosum_set"]:
            self.registerPropagator(prop_type=amosum.prop_type, id=amosum.id)


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
        try:
            handle : clingo.SolveHandle = self.ctl.solve(on_model=on_model, async_ = True)
            res = handle.wait(self.timeout_m * 60 if not self.exp else None)
        except Exception as e:
            print(e)
        end_time = time.time()  # End time

        for propagator in self.propagators:
            amosum_propagator = propagator.propagators[0]
            self.update_maps_weights_list(input = amosum_propagator)

        regex_name_file = r"(.*\/)?(?P<name>.+)\.asp"
        encoding_name = re.match(regex_name_file, location_encoding).group("name")  if encoding else ""
        instance_name = re.match(regex_name_file, location_instance).group("name")
        filename = f"{STATISTICS_RUN}/{date_string}-{encoding_name}-{instance_name}"
        self.print_stats_to_file(filename=filename)
        
        wall_time = end_time - start_time
        wall_time = round(wall_time, 2) if res else "timeout"

        # restoring the instance.asp file
        self.comment_bound(instance=instance, ub=False, restore=True)
        self.comment_bound(instance=instance, ub=True, restore=True)
        
        return models, wall_time
    
    def update_maps_weights_list(self, input: AmoSumPropagator):

        id = input.ID
        maps_weights = input.weights_names
        self.maps_weights_list.append((id, maps_weights))
    
    def registerPropagator(self, prop_type: str, id: str):
        ge, propagate_phase, prop_type = get_propagator_variables(prop_type=prop_type)

        # Initialize and register the custom propagator
        param = self.param.copy()
        param["id"] = id
        propagator_clingo = PropagatorClingo(param, propagation_phase=propagate_phase, ge=ge, prop_type=prop_type)
        self.ctl.register_propagator(propagator_clingo)
        self.propagators.append(propagator_clingo)


    def print_stats_to_file(self, filename):
        if not self.param.get("stats", False):
            return
        control = self.ctl
        stats = control.statistics  # Access statistics object
        with open(filename, "w") as file:
            # Pretty-print the stats as JSON to make it readable
            json.dump(stats, file, indent=4)
            print(f"Statistics written to {filename}")
