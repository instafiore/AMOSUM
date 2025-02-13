#!/usr/bin/python3

import re
import xml.etree.ElementTree as ET
import sys
import os
from os import system
sys.path.append(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
from experiments.check_correctness_sat import check_satisfability
from experiments.check_correctness_unsat import check_unsatisfability
from experiments.setup import *

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

    file = param.get("f")
    
    # checking for satisfability
    if(param.get("sat", False)):
        output = cat(file.strip())
        print("checking correctness for satisfability..")
        sat, encoding_checker_path, instance = check_satisfability(file, output=output)
        if not sat:
            print(f"failed satness for {file} with encoding: {encoding_checker_path} instance: {instance}")
            exit(1)
        else:
            print(f"Success satisfability for {file} :)") 
            exit(0)

    elif(param.get("unsat", False)):
        print("checking correctness for unsatisfability..")
        unsat = check_unsatisfability(file)
        if not unsat:
            print(f"failed unsatness for {file}")
            exit(1)
        else:
            print(f"Success unsatness for {file} :)") 
            exit(0)

if __name__ == "__main__":
    main()