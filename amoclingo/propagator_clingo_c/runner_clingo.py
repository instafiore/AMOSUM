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

        self.create_bound(instance=instance, ub=False)
        self.create_bound(instance=instance, ub=True)
        

        hidden_location_encoding= self.rewrite_file_without_amosum(location_encoding)
        hidden_location_instance= self.rewrite_file_without_amosum(location_instance)

        
        grounded_program, run_command_ground = ground_program(hidden_location_encoding, hidden_location_instance, return_command=True)
        
        run = f"{timeout_str} {settings.PROPAGATOR_DIR_LOCATION_CLINGO_C_BIN}/./{RunnerClingoC.SOLVER_EXE} {self.n0}\
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
            print(f"run:\t {run}") if not self.exp else None

        # running test
        self.maps_weights_list = []
        # compile propagator
        if not self.exp or self.param.get("clean",False) : self.compile()


        try:
            run_process = subprocess.run(run, shell=True, capture_output=True, text=True)
        except KeyboardInterrupt as e:
            print("Interrupted")
            return [], -1, preprocess_map["amosum_mapweights"]

        output = run_process.stdout
        error = run_process.stderr
        output_error = output + error
    
        lines_output = output.splitlines() 
        lines_error = error.splitlines() 
        
        output = output.strip()
        
        # if RunnerWasp.PRINT_OUTPUT_SOLVER and output != "" :
        #     print(f"{output}")

        avoiding_time_information_regex = r"(real \d+\.\d+|user \d+\.\d+|sys \d+\.\d+)"
        error = re.sub(avoiding_time_information_regex, "", error, count=0, flags=0).strip()
        if RunnerWasp.PRINT_ERROR_SOLVER and error != "":
            print(error, file=sys.stderr)

        regex_real = r"^real\s(\d+\.\d+)"
        # regex for the answer set of a given problem
        regex_answer_set = r"^Answer set \{(.+)\}"
        
        # regex for the interested atoms of a given problem
        regex_query = self.get_regex_query_atom_answerset()

        answer_sets = []

        # if RunnerWasp.PRINT_OUTPUT_SOLVER:
        #         print(output)

        for line in lines_output:
            if not re.search(regex_answer_set, line) is None:
                answer_set_str = re.search(regex_answer_set, line).group(1)
                # print(f"find all: {re.findall(regex_query, answer_set_str)}")
                answer_set = set([match[0] for match in re.findall(regex_query, answer_set_str)]) if self.problem != RunnerWasp.NPD else answer_set_str.split(", ")
                # print(f"line:{line} regex_query: {regex_query} answer_set:{answer_set}")
                answer_sets.append(set(answer_set))
            elif RunnerWasp.PRINT_OUTPUT_SOLVER:
                print(line)
        
        time = "error" if not self.exp else "experiment mode"
        for line in lines_error:
            if not re.search(regex_real, line) is None:
                time = re.search(regex_real, line).group(1)
            elif not re.search(r"Killed: Bye!", line) is None:
                time = "timeout"

            self.update_maps_weights_list(input = line)

        self.comment_bound(instance=instance, ub=False, restore=True)
        self.comment_bound(instance=instance, ub=True,  restore=True)

        return answer_sets, time, preprocess_map["amosum_mapweights"]
    

    def compile(self):
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