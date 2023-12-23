#!/usr/bin/python3

import subprocess
import sys
import os
import re
sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..')))
import settings
assert len(sys.argv) >= 2

problem = sys.argv[1]
lb = sys.argv[2]
light = sys.argv[3]

if(len(sys.argv) > 4):
    number_of_tests = int(sys.argv[4])
else:
    number_of_tests = -1

head = ""
if number_of_tests != -1:
    head = f"| head -n {number_of_tests}"

output = subprocess.run(f"ls {settings.BENCHMARKS_LOCATION}/{problem}/instances {head}", shell=True, capture_output=True, text=True)

output = output.stdout.splitlines()

timestamp = subprocess.run('date +"%Y-%m-%d.%H.%M.%S"', shell=True, capture_output=True, text=True).stdout
print(timestamp)
for line in output:
    regex_asp_program = r"(.+?)\.asp" 
    res_regex = re.match(regex_asp_program, line)
    if not res_regex is None:
        instance = res_regex.group(1)
        subprocess.run(f'{settings.TESTS_LOCATION}/run_test.py {problem} {instance} {lb} {light} {timestamp}', shell=True)
