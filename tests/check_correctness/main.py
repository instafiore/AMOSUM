#!/usr/bin/python3

import re
import xml.etree.ElementTree as ET
import sys
import os
from os import system

sys.path.append(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

from check_correctness.check_correctness_sat import check_satisfability, run_check
from check_correctness.check_correctness_unsat import check_unsatcorrectness
from check_correctness.setup import get_all_commands, setup

def init_run(argv):
    param = {}
    regex = r"^-(.+)" 
    
    i = 1
    while i < len(argv):
        
        # creating the key
        key = argv[i] 
        res_regex = re.match(regex, key)
        if res_regex is None:
            raise Exception("Every key has to start with a dash! Ex: -problem knapsack")
        key = res_regex.group(1)

        if i + 1 >= len(argv) :
            param[key] = True
            break
        
        value = argv[i+1] 
        res_regex = re.match(regex, value)
        if res_regex is None:
            i += 2
            param[key] = value
            
        else:
            i += 1
            param[key] = True

    return param


def cat(filename):
    with open(filename, 'r') as file:
        return file.read()
    
def main():

    param = init_run(sys.argv)

    output = ""
    file = param.get("f")
    output = cat(file.strip())
    
    

    map = setup()
    # # checking for unsatness
    # if(param.get("unsat", False)):
    #     raise NotImplementedError()
    #     print("checking correctness for unsatness..")
    #     print(get_all_commands(file=merged_file)) 
    #     res_unsat = check_unsatcorrectness(root=root, map_inconsistent_regex=map["map_inconsistent_regex"])
    #     print("Success unsatness :)") if  res_unsat else print("Unsuccess unsatness :(")

    # checking for satisfability
    if(param.get("sat", False)):
        print("checking correctness for satisfability..")
        res_sat = check_satisfability(file, output=output, 
                                    map_regex_satisfability=map["map_sat_regex"],
                                    map_regex_findall_answerset=map["map_regex_findall_answerset"])
        print(f"Success satisfability for {file} :)") if  res_sat else print(f"Unsuccess satisfability for {file} :(")

if __name__ == "__main__":
    main()