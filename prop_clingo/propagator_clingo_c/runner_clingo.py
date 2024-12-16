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
from prop_wasp.runner_wasp import RunnerWasp
from  utility import *
import utility
import settings
from preprocess import *
from datetime import datetime

class RunnerClingoC(RunnerWasp):
    '''
    This class is meant to run experiments on the AMO sum propagator(s) Clingo using C API
    '''

    SOLVER = "propagator_clingo"
    

    def __init__(self, parameters: Dict[str, str]) -> None:
        super().__init__(parameters)
    
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

        print(f"encoding: {location_encoding}")
        print(f"instance: {location_instance}")

        timeout_str = f"timeout {self.timeout_m}m" if not self.exp else ""

        hidden_location_encoding= self.rewrite_file_without_amosum(location_encoding)
        hidden_location_instance= self.rewrite_file_without_amosum(location_instance)
        
        grounded_program, run_command_ground = ground_program(hidden_location_encoding, hidden_location_instance, return_command=True)
        
        run = f"{timeout_str} time -p {settings.PROPAGATOR_DIR_LOCATION_CLINGO_C_BIN}/./{RunnerClingoC.SOLVER} {self.n0}\
            -enc={hidden_location_encoding}\
            -i={hidden_location_instance}"

        preprocess_map =  preprocess_ground_program(grounded_program)
        for amosum in preprocess_map["amosum_set"]:
            self.registerPropagator(amosum.prop_type, amosum.id)

        prop_run = ""
        if len(preprocess_map["amosum_set"]) > 0:
            prop_run = f" -amosum_propagator=\"{' '.join(self.propagators)}\" "
            run += prop_run

        if self.PRINT_RUN:
            print(f"run:\t{run_command_ground} | {run}")

        # running test
        self.maps_weights_list = []
        # compile propagator
        compile_run = f"make -C {PROPAGATOR_DIR_LOCATION_CLINGO_C}"
        print(compile_run)
        subprocess.run(compile_run, shell=True)
        run_process = subprocess.run(run, shell=True, capture_output=True, text=True)

        output = run_process.stdout
        error = run_process.stderr
        output_error = output + error

        lines_output = output.splitlines() 
        lines_error = error.splitlines() 
        
        output = output.strip()
        
        if RunnerWasp.PRINT_OUTPUT_SOLVER and output != "" :
            print(f"{output}")

        avoiding_time_information_regex = r"(real \d+\.\d+|user \d+\.\d+|sys \d+\.\d+)"
        error = re.sub(avoiding_time_information_regex, "", error, count=0, flags=0).strip()
        if RunnerWasp.PRINT_ERROR_SOLVER and error != "":
            print(error, file=sys.stderr)

        return [], 0