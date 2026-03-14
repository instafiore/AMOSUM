from io import StringIO
import json
import math
import subprocess
import time
from typing import Dict, List
import re
import sys
import os

from clingo import SolveResult
import amosum.AmoSumParser
from amosum.amosum import *
from clingo.symbol import Number
from clingo.control import Control

# adding root path
sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..')))

import ast
from amosum.amowasp.propagator_wasp_py.runner_wasp import RunnerWasp
from  utility import *
import utility
import settings
from amosum.amoclingo.propagator_clingo_py.propagator_clingo import *
from preprocess import *
from datetime import datetime

class RunnerClingoPython(RunnerWasp):
    '''
    This class is meant to run experiments on the AMO sum propagator(s) Clingo using Python API
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
        arguments.append("--stats=2") if self.param.get("stats", False) else None
        # arguments.append("--configuration=tweety")
    
        self.ctl = Control(arguments=arguments)
        self.propagators: List[PropagatorClingo] = []
        # Load the instance file

        # Rewrinting without #amosum construct

        hidden_location_encoding= self.rewrite_file_without_amosum(location_encoding)
        hidden_location_instance= self.rewrite_file_without_amosum(location_instance)
        # print(f"grounded file:\n {cat(hidden_location_encoding)}")
        grounded_program = ground_program(hidden_location_encoding, hidden_location_instance)
        # print(f"grounded_program: {grounded_program}")

        preprocess_map = preprocess_ground_program(grounded_program)
        self.ctl.load(hidden_location_encoding) if encoding else ""
        self.ctl.load(hidden_location_instance)
        self.ctl.ground([("base", [])])  # Ensure you ground with the correct subprogram
        # print(f"preprocess_map: {preprocess_map}")

        
        for amosum in preprocess_map["amosum_set"]:
            self.registerPropagator(prop_type=amosum.prop_type, id=amosum.id)

        model : Model = None
        def on_model(modelClingo):
            nonlocal model
            model = Model(None, [str(atom) for atom in modelClingo.symbols(shown=True)])

    
        result: Result = None
        def on_finish(x: SolveResult):
            nonlocal result
            if x.unsatisfiable:
                exitCode = 20
            elif x.satisfiable:
                exitCode = 10
            else:
                exitCode = 1
            result = Result(model, exitCode)

        try:
            handle : clingo.SolveHandle = self.ctl.solve(on_finish=on_finish ,on_model=on_model, async_ = True)
            handle.wait()
        except Exception as e:
            result = Result(None, 1)
            raise e
        
        print(result)
        
        return result.exitCode
    
    def update_maps_weights_list(self, input: AmoSumPropagator):

        if not input.param.get("check_mps", False):
            return
        id = input.ID
        maps_weights = input.weights_names
        self.maps_weights_list.append((id, maps_weights))
    
    def registerPropagator(self, prop_type: str, id: str):
        ge, propagate_phase, choice_cons = get_propagator_variables(prop_type=prop_type)

        # Initialize and register the custom propagator
        param = self.param.copy()
        param["id"] = id
        propagator_clingo = PropagatorClingo(param, propagation_phase=propagate_phase, ge=ge, choice_cons=choice_cons)
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
