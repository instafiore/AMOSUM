
import os
import re
import subprocess
from typing import List
import xml.etree.ElementTree as ET
import sys

from experiments.setup import *
from pathlib import Path

def run_check(answer_set: List[str], instance):
    instance_checker = []
    for atom in answer_set:
        instance_checker.append(f"{atom}.")
    path = Path(instance)
    folder = path.parent
    bench = Path(folder).name.split(".")[0]
    instance = path.name
    encoding_checker_path = f"./instances/{bench}/encoding-checker.asp"
    instance_checker_path = f"/tmp/.{instance}_answerset.asp"
    instance_checker = "\n".join(instance_checker)
    with open(instance_checker_path, "w") as file:
        file.write(instance_checker)

    print(f"running {encoding_checker_path} {instance_checker_path}")
    run_clingo = subprocess.run(f"clingo {encoding_checker_path}  {instance_checker_path}", shell=True, capture_output=True, text=True)
    output = run_clingo.stdout
    error = run_clingo.stderr
    output_error = output + error

    res = re.search(r"UNSATISFIABLE",output_error) is None
    if not res:
        print(output_error)
    if instance_checker_path: os.remove(instance_checker_path)
    return res, encoding_checker_path, path


def check_satisfability(instance, output):

    res_sat = True
    output_match = re.search(REGEX_SAT, output)
    if output_match:
        answerset_string = output_match.group("answerset")
        answer_set = re.findall(REGEX_FINDALL_ANSWERSET, answerset_string)
        assert(len(answer_set) > 0)
        return run_check(answer_set, instance)
        
    return res_sat, None, None