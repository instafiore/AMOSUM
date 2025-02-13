
import os
import re
import subprocess
from typing import List
import xml.etree.ElementTree as ET
import sys

from experiments.setup import *
from pathlib import Path
import pandas as pd

def check_unsatisfability(file):
    df = pd.read_csv(file, delimiter='\t', names=["instance", "solver", "type", "version", "lang", "lazy", "min", "full_instance", "bho1", "bho2", "status", "exit_code", "real", "time", "user", "bho3", "memory"])
    
    for version in ["amo", "eo"]:
        print(f"checking correctness for unsatisfability for version: {version}..")
        df_amosum_unsat = df[ (df["type"] == "amosum") & (df["exit_code"] ==  20) & (df["version"] == f"{version}")] 
        df_plain_sat = df[(df["type"] == "plain") & (df["exit_code"] == 10) & (df["version"] == f"{version}")]

        amosum_instances = set(df_amosum_unsat["instance"])
        plain_instances = set(df_plain_sat["instance"])
        
        # Find common instances (should be empty for a valid check)
        common_instances = amosum_instances.intersection(plain_instances)

        if common_instances:
            print(f"Error: Some instances appear in both df_amosum_unsat and df_plain_sat! for version: {version}")
            print(common_instances) 
            return False 
    
    return True  

    