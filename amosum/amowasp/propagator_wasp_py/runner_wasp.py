import json
import subprocess
from typing import Dict, List
import re
import sys
import os
import ast
from  utility import *
import utility
from preprocess import *
from amosum.amosum_parser.__main__ import run as run_rewriter
sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..')))
import settings
from datetime import datetime





class RunnerWasp:
    '''
    This class is meant to run experiments on the AMO sum propagator(s)
    '''

    # the solver that you are using
    SOLVER = "wasp_python"
    

    def __init__(self, parameters: Dict[str,str]) -> None:

        self.param = parameters
        self.maps_weights_list = [] 
        self.tmp_files = []
        
        self.propagators = []
    
        self.num_models = self.param.get("models")  

        self.encoding = self.param.get("encoding")
        self.instance = self.param.get("instance", None)



    def run(self):


        # encoding
        location_encoding = self.encoding
        location_instance = self.instance
       
        print(f"encoding: {location_encoding}")
        print(f"instance: {location_instance}")

        hidden_location_encoding= self.rewrite_file_without_amosum(location_encoding)
        hidden_location_instance= self.rewrite_file_without_amosum(location_instance)
        
        grounded_program, run_command_ground = ground_program(hidden_location_encoding, hidden_location_instance, return_command=True)
        
        run = f"{RunnerWasp.SOLVER}"
        

        # preprocessing
        preprocess_map =  preprocess_ground_program(grounded_program)
        for amosum in preprocess_map["amosum_set"]:
            self.registerPropagator(amosum.prop_type, amosum.id)

        prop_run = ""
        if len(preprocess_map["amosum_set"]) > 0:
            prop_run = f" --interpreter=python \
            --script-directory={settings.PROPAGATOR_DIR_LOCATION_WASP} \
            --plugins-file=\"{settings.PROPAGATOR_MODULE} {' '.join(self.propagators)}\" "
            run += prop_run
  
        print(f"run:\t{run_command_ground} | {run}")


        run_process = subprocess.run(run, input=grounded_program ,shell=True, executable="/bin/bash", text=True)

        return run_process.returncode

    def registerPropagator(self, prop_type: str, id: str):
        
        string_param_list = []
        for key in self.param:
            if self.param[key] == True:
                string_param = f"-{key}"
            else:
                string_param = f"-{key} {self.param[key]}"
            string_param_list.append(string_param)

        params_str = " ".join(string_param_list)
        prop = f"{prop_type} -id {id} {params_str}"
        self.propagators.append(prop)
     

    def rewrite_file_without_amosum(self, file):
        now = datetime.now()
        date_string = now.strftime("%Y-%m-%d-%H-%M-%S-%f")
        file_name = re.search(FILE_REGEX, file).group("file_name")
        # print(f"file_name: {file_name}")
        file_name = re.search(r"(.*)\.asp", file_name).group(1)
        non_ground_file_without_amosum = run_rewriter(input=file)
        # print(f"non_ground_encoding_without_amosum\n{non_ground_file_without_amosum}")
        hidden_file_without_amosum = f".{file_name}_without_amosum_{date_string}.asp"
        hidden_file_without_amosum_tmp_location= f"/tmp/{hidden_file_without_amosum}"
        write_file(hidden_file_without_amosum_tmp_location, non_ground_file_without_amosum)
        self.tmp_files.append(hidden_file_without_amosum_tmp_location)
        return hidden_file_without_amosum_tmp_location
    
    def __del__(self):
        for file in self.tmp_files:
            # debug(f"removing file {file}", force_print=True)
            # os.remove(file)
            pass