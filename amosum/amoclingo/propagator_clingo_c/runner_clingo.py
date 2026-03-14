from io import StringIO
import json
import math
import subprocess
import time
from typing import Dict, List
import re
import sys
import os
import amosum_parser
from amosum import *
import signal

# adding root path
sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..')))

import ast
from amowasp.propagator_wasp_py.runner_wasp import RunnerWasp
from  utility import *
import utility
import settings
from preprocess import *
from datetime import datetime



        

class RunnerClingoCpp(RunnerWasp):
    '''
    This class is meant to run experiments on the AMO sum propagator(s) Clingo using C API
    '''

    SOLVER_EXE = "amosum_cpp"
    

    def __init__(self, parameters: Dict[str, str]) -> None:
        super().__init__(parameters)

    def handleRun(self, run, weights, totalTime) -> Result:
        eachModelTime = totalTime
        generator = run_and_stream(run)
        result: Result = None
        alreadyPrinted = False
        maximizationProblem = not weights is None

        for line in generator:
            # print(f"serialized: {line}")
            if not line.strip(): continue
            resultNew = Result.parse(line, weights)
            if resultNew is None:
                continue
            result = resultNew
            if maximizationProblem:
                endCurrentModelTime = time.time()
                result.cumulativeTime = round(endCurrentModelTime - totalTime,3)
                result.timeModel = round(endCurrentModelTime - eachModelTime, 3)
                eachModelTime = endCurrentModelTime            
            print(result)
        
        return result
    
    def run(self):
        
        location_encoding = self.encoding
        location_instance = self.instance

        print(f"encoding: {location_encoding}") 
        print(f"instance: {location_instance}")

        
        hidden_location_encoding= self.rewrite_file_without_amosum(location_encoding)
        hidden_location_instance= self.rewrite_file_without_amosum(location_instance)

        
        grounded_program, run_command_ground = ground_program(hidden_location_encoding, hidden_location_instance, return_command=True)
        
        run = f"{settings.PROPAGATOR_DIR_LOCATION_CLINGO_C_BIN}/./{RunnerClingoCpp.SOLVER_EXE} {self.n0}\
            -encoding={hidden_location_encoding}\
            -instance={hidden_location_instance} \
            -serialize"

        preprocess_map =  preprocess_ground_program(grounded_program)
        for amosum in preprocess_map["amosum_set"]:
            self.registerPropagator(amosum.prop_type, amosum.id)

        prop_run = ""
        if len(preprocess_map["amosum_set"]) > 0:
            prop_run = f" -amosum_propagator=\"{' '.join(self.propagators)}\" "
            run += prop_run

        # print(f"weights: {str(preprocess_map["amosum_mapweights"])}")
        weights = preprocess_map["amosum_mapweights"].get("__amomaximizeid__",None) # Decomment to have cost only for amomaximize problems
        # weights = preprocess_map["amosum_mapweights"]

     
        print(f"run:\t {run}")

        totalTime = time.time()
        result = None
        
        result = self.handleRun(run, weights, totalTime)
        print(f"Exit code: {result.exitCode}")
        
        return result.exitCode if result else 40
