#!/usr/bin/python3

import subprocess
import sys
import os
import re
sys.path.append(os.environ.get('WASP_HOME'))
import propagator_opt_dir.settings as settings


PROBLEM = sys.argv[1]
INSTANCE = sys.argv[2]
LB = sys.argv[3]
light = sys.argv[4]
TIMESTAMP = sys.argv[5] if len(sys.argv) >= 6 else subprocess.run('date +"%Y-%m-%d.%H.%M"', shell=True, capture_output=True, text=True).stdout
TIMESTAMP = TIMESTAMP.splitlines()[0]
if light == "1":
    light = True
else:
    light = False

WEIGHTS = "weights"

light_print = "light" if light else "normal"
print(f"{PROBLEM} {INSTANCE} {TIMESTAMP} {LB} {light_print}")


subprocess.run(f"echo 'lb({LB},0).' > {settings.BENCHMARKS_LOCATION}/{PROBLEM}/lb.asp", shell=True)

# gringo benchmarks/simple_tests/test.asp --output=smodels | wasp --interpreter=python --script-directory=/home/instafiore/git/wasp/propagator_opt_dir --plugins-file=propagator_opt -n0

answer_sets_aggr : list
answer_sets_group : list

regex_instance_file : str = r"^(?P<number>\d+)-(?P<problem>\w+)-(?P<number_nodes>\d+)-(\d+)"

groups = re.search(regex_instance_file, INSTANCE)

number = groups.group("number")
problem = groups.group("problem")
number_nodes = groups.group("number_nodes")

time_group : float
time_aggr : float

silent = ""
# silent = "--silent=2"
for encoding  in (  
                    # settings.ENCODING_WITH_GROUP_E, 
                    settings.ENCODING_WITH_GROUP_LE, 
                    settings.ENCODING_WITH_AGGR
                    ):
    
    group_le = encoding == settings.ENCODING_WITH_GROUP_LE
    group_e = encoding == settings.ENCODING_WITH_GROUP_E
    group = group_le or group_e

    # print(f"started {encoding}")

    location = f"{settings.BENCHMARKS_LOCATION}/{problem}"
    if light:
        location_instance = f"{settings.BENCHMARKS_LOCATION}/{problem}/light"
    else:
        location_instance = f"{settings.BENCHMARKS_LOCATION}/{problem}/instances"

    run = f"clingo \
        {location_instance}/{INSTANCE}.asp \
        {location}/{WEIGHTS}.asp \
        {location}/{encoding}.asp \
        {location}/lb.asp  \
        --output=smodels | timeout 20m time -p wasp {silent} -n0 "
    
    if group_e :
        run += f"--interpreter=python \
        --script-directory={settings.PROPAGATOR_DIR_LOCATION} \
        --plugins-file={settings.PROPAGATOR_NAME_e} "
    elif group_le:
        run += f"--interpreter=python \
        --script-directory={settings.PROPAGATOR_DIR_LOCATION} \
        --plugins-file={settings.PROPAGATOR_NAME_le} "

    # print("\nRUN\n")
    # print(run)
    # print("\n")
        
    output = subprocess.run(run, shell=True, capture_output=True).stdout.decode() + \
        subprocess.run(run, shell=True, capture_output=True).stderr.decode()
    lines = output.splitlines() 

    regex_real = r"^real\s(\d+\.\d+)"
    regex_answer_set = r"\{(.+)\}"
    regex_query = r"(?<=[\s,])col\(\w+,\w+?\)"
    answer_sets = []
    for line in lines:
        
        if not re.search(regex_real, line) is None:
            time = re.search(regex_real, line).group(1)
            if group:
                time_group = time
            else:
                time_aggr = time
        elif not re.search(regex_answer_set, line) is None:
            answer_set = re.search(regex_answer_set, line).group(1)
            answer_set = set(re.findall(regex_query, answer_set))
            answer_sets.append(set(answer_set))
        elif not re.search(r"Killed: Bye!", line) is None:
            time = "timeout"
            if group:
                time_group = time
            else:
                time_aggr = time

    if group:     
        answer_sets_group = answer_sets
    elif encoding == settings.ENCODING_WITH_AGGR:
        answer_sets_aggr = answer_sets

    # print(f"{encoding} {time} {len(answer_sets)}")

ng = len(answer_sets_group)
na = len(answer_sets_aggr)

if na != ng:
    correct = False
else:
    correct = True
    for ans_1 in answer_sets_aggr:
        if not ans_1 in answer_sets_group:
            correct = False
            print(ans_1)
            break

equal = "equal" if correct else "not_equal" 

if not correct:
    print("DIFFERENT")
new_line = f"{number},{problem},{number_nodes},{time_aggr},{time_group},{na},{ng},{equal},{LB}"
subprocess.run(f"echo '{new_line}' >> {settings.RESULTS_TESTS_LOCATION}/{PROBLEM}.{TIMESTAMP}.res ", shell=True, capture_output=True)

