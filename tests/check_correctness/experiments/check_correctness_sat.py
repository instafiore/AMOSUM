
import re
import subprocess
from typing import List
import xml.etree.ElementTree as ET
import sys

from check_correctness.experiments.setup import *
from pathlib import Path

def run_check(answer_set: List[str], instance):
    instance_checker = []
    for atom in answer_set:
        instance_checker.append(f"{atom}.")
    path = Path(instance)
    search_res = re.search(REGEX_INSTANCE, instance)
    folder = path.parent
    instance = path.name
    encoding_checker_path = f"{folder}/encoding-checker.asp"
    instance_checker_path = f"check_correctness/instances/{instance}_answerset.asp"
    instance_checker = "\n".join(instance_checker)
    with open(instance_checker_path, "w") as file:
        file.write(instance_checker)

    run_clingo = subprocess.run(f"clingo {encoding_checker_path}  {instance_checker_path}", shell=True, capture_output=True, text=True)
    output = run_clingo.stdout
    error = run_clingo.stderr
    output_error = output + error

    res = re.search(r"UNSATISFIABLE",output_error) is None
    if not res:
        print(output_error)
    return res, encoding_checker_path, instance_checker_path






def check_satisfability(instance, output):

    res_sat = True
    output_match = re.search(REGEX_SAT, output)
    if output_match:
        answerset_string = output_match.group("answerset")
        answer_set = re.findall(REGEX_FINDALL_ANSWERSET, answerset_string)
        assert(len(answer_set) > 0)
        sat, encoding_checker_path, instance_checker_path = run_check(answer_set, instance)
        if not sat:
            res_sat = False
            print(f"failed satness with encoding_checker_path: {encoding_checker_path} instance_checker_path: {instance_checker_path}")
            exit(1)
    return res_sat