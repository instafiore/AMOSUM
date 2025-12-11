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

# adding root path
sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..')))

import ast
from amowasp.propagator_wasp_py.runner_wasp import RunnerWasp
from  utility import *
import utility
import settings
from preprocess import *
from datetime import datetime


        

class RunnerClingoC(RunnerWasp):
    '''
    This class is meant to run experiments on the AMO sum propagator(s) Clingo using C API
    '''

    SOLVER_EXE = "runner_clingo"
    

    def __init__(self, parameters: Dict[str, str]) -> None:
        super().__init__(parameters)

    def handleRun(self, run, weights, totalTime) -> Result:
        eachModelTime = totalTime
        generator = run_and_stream(run)
        result: Result
        alreadyPrinted = False
   
        try:
            for line in generator:
                # print(f"New line {line}")
                if not line.strip(): continue
                result = Result.parse(line, weights)
                if result is None:
                    continue
                endCurrentModelTime = time.time()
                result.cumulativeTime = round(endCurrentModelTime - totalTime,3)
                result.timeModel = round(endCurrentModelTime - eachModelTime, 3)
                print(result)
                # print(f"Result normal: {result}")
                eachModelTime = endCurrentModelTime

                # if result.exitCode == 29 or result.exitCode == 30:
                #     alreadyPrinted = True
            
            # result.exitCode = 30
        except KeyboardInterrupt as e:
            
            for line in generator:
                # print(f"New line {line}")
                if not line.strip(): continue
                resultNew = Result.parse(line, weights)
                if resultNew is None:
                    continue
                result = resultNew
                endCurrentModelTime = time.time()
                result.cumulativeTime = round(endCurrentModelTime - totalTime,3)
                result.timeModel = round(endCurrentModelTime - eachModelTime, 3)
                # print(f"Result key: {result}")
                print(result)
                eachModelTime = endCurrentModelTime

                # if result.exitCode == 29 or result.exitCode == 30:
                #     alreadyPrinted = True
            
            # result.exitCode = 29

        # if not alreadyPrinted:
        #     print(result)
        
        return result
    
    def run_instance(self, instance, encoding=None):
        
        # Get the current date and time
        now = datetime.now()

        # Format it as a string
        date_string = now.strftime("%Y-%m-%d-%H-%M")

        # Setup encoding and instance file paths
        location_encoding = f"{self.location}/{encoding}.asp" if encoding else ""
        location_encoding = encoding if self.enc else  location_encoding
        location_encoding = f"{self.location}/{encoding}.asp" if self.enc and not self.exp else location_encoding
        location_instance = f"{self.location_instance}/{instance}.asp" if not self.exp else instance

        timeout_str = f"timeout {self.timeout_m}m time -p " if not self.exp else ""

        print(f"encoding: {location_encoding}") if not self.exp  else None
        print(f"instance: {location_instance}") if not self.exp  else None

        # self.create_bound(instance=instance, ub=False)
        # self.create_bound(instance=instance, ub=True)
        
        hidden_location_encoding= self.rewrite_file_without_amosum(location_encoding)
        hidden_location_instance= self.rewrite_file_without_amosum(location_instance)

        
        grounded_program, run_command_ground = ground_program(hidden_location_encoding, hidden_location_instance, return_command=True)
        
        run = f"{timeout_str} {settings.PROPAGATOR_DIR_LOCATION_CLINGO_C_BIN}/./{RunnerClingoC.SOLVER_EXE} {self.n0}\
            -enc={hidden_location_encoding}\
            -i={hidden_location_instance} \
            -serialize"

        preprocess_map =  preprocess_ground_program(grounded_program)
        for amosum in preprocess_map["amosum_set"]:
            self.registerPropagator(amosum.prop_type, amosum.id)

        prop_run = ""
        if len(preprocess_map["amosum_set"]) > 0:
            prop_run = f" -amosum_propagator=\"{' '.join(self.propagators)}\" "
            run += prop_run

        # print(f"weights: {str(preprocess_map["amosum_mapweights"])}")
        # weigths = preprocess_map["amosum_mapweights"].getdefault("__amomaximizeid__",None)
        weights = preprocess_map["amosum_mapweights"]

        if self.PRINT_RUN:
            print(f"run:\t {run}")

        # running test
        # self.maps_weights_list = []
        # compile propagator
        if not self.exp or self.param.get("clean",False) or self.param.get("make",False)  : self.compile()

        totalTime = time.time()
        result = None
        

        result = self.handleRun(run, weights, totalTime)
        print(f"Exit code: {result.exitCode}")
        
        return result.exitCode if result else 40
    

    def compile(self):
        make = self.param.get("make",False) 
        clean = self.param.get("clean",False) 
        clean = clean or self.param.get("d",False) or self.param.get("check_mps",False) or self.param.get("sanitize",False)
        compile_with_debug = "DEBUG=-DDEBUG" if self.param.get("d",False) else ""
        compile_with_check_mps = "CHECK_MPS=-DCHECK_MPS" if self.param.get("check_mps",False) else ""
        compile_with_sanitize = 'SANITIZE_ADDRESS="-fsanitize=address -g"' if self.param.get("sanitize", False) else ""
        compile_with_sanitize = 'SANITIZE_ADDRESS=-g' if not self.param.get("sanitize", False) and self.param.get("g", False) else compile_with_sanitize
        clean_run = f"make -C {PROPAGATOR_DIR_LOCATION_CLINGO_C} clean"
        compile_run = f"make -C {PROPAGATOR_DIR_LOCATION_CLINGO_C} {compile_with_debug} {compile_with_check_mps} {compile_with_sanitize}"
        # print(compile_run)
        subprocess.run(clean_run, shell=True) if clean else ""
        subprocess.run(compile_run, shell=True)