#!/usr/bin/python3

import re
import xml.etree.ElementTree as ET
import sys

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


def check_unsatcorrectness(root: ET.Element, regex_instance, commands_to_check, map_inconsistent_regex, 
                           base_command = "clingo_plain", killed_regex = r"kill"):

    unsat_instances_base = []
    timeout_instances_base = []

    def compute_unsat_instances(command_id):   
        unsat_instances = set()
        timeout_instances = set()
        present = False
        for test_case in root.findall('.//testcase'):
            command = test_case.find("command")
            if command.attrib["id"] == command_id:
                present = True
                unsat = False
                timeoout = False
                for stream in command.iter("stream"):
                    line = stream.find("line")
                    line_text = line.text
                    if line_text and stream.attrib.get("type", False) == "stderr":
                        if re.search(killed_regex, line_text, flags=re.IGNORECASE):
                            timeoout = True
                            
                    elif line_text and stream.attrib.get("type", False) == "stdout": 
                        if re.search(map_inconsistent_regex[command_id], line_text):
                            unsat = True
                            break
                        
                instance = re.search(regex_instance, test_case.attrib["id"]).group("instance")
                
                if timeoout:
                    timeout_instances.add(instance)
                elif unsat:
                    unsat_instances.add(instance)
                

        return unsat_instances, timeout_instances, present

    unsat_instances_base, timeout_instances_base, present = compute_unsat_instances(base_command)
    if not present:
        raise Exception(f"Base command {base_command} not present")
    correct = True
    for command in commands_to_check:
        unsat_instances_command, timeout_instances_command, present = compute_unsat_instances(command_id=command)
        if not present:
            continue
        
        # print(f"command:{command} \nunsat_instances_command: {unsat_instances_command} \ntimeout_instances_command: {timeout_instances_command}")
       
        # one way: if unsat base then unsat command
        for instance_base in unsat_instances_base:
            if instance_base not in unsat_instances_command and \
                instance_base not in timeout_instances_command:
                print(f"instance base: {instance_base} not in command: {command}", file=sys.stderr)
                correct = False
        
        # second way: if unsat command then unsat base
        for instance_command in unsat_instances_command:
            if instance_command not in unsat_instances_base and \
                instance_command not in timeout_instances_base:
                print(f"instance command {command}: {instance_command} not in base: {base_command}", file=sys.stderr)
                correct = False

        
    return correct

def get_instance_info(regex, line):
    match = re.match(regex, line)
    instance = match.group("instance")

    try:
        lb = int(match.group("lb"))
    except Exception as e:
        lb = None

    try:
        ub = int(match.group("ub"))
    except Exception as e:
        ub = None

    return (instance, lb, ub)

def check_satisfability(root: ET.Element, regex_instance, commands_to_check, map_regex_satisfability, ge):

    def check_satisfability_for_command(command_id):
        
        res_sat = True
        for test_case in root.findall('.//testcase'):
            command = test_case.find("command")
            if command.attrib["id"] != command_id:
                continue
            instance, lb, ub = get_instance_info(regex=regex_instance, line =test_case.attrib["id"])
            output = ""
            for stream in command.iter("stream"):
                if stream.attrib.get("type", False) == "stdout":
                    line = stream.find("line")
                    output += f"{line.text}\n" if line.text else ""

            output_match = re.search(map_regex_satisfability[command.attrib["id"]], output)
            if output_match:
                mps = int(output_match.group("mps"))
                check = mps < lb if ge else mps > ub 
                b = lb if ge else ub
                constraint = "lower" if ge else "greater"
                # print(f"[{command_id}] {instance} has a mps equal to {mps}")
                if check:
                    print(f"{instance} has a {mps} {constraint} than {b}")
                    res_sat = False
        
        return res_sat
    
    satisfability = True
    for command in commands_to_check:
        if not check_satisfability_for_command(command):
            satisfability =  False

    return satisfability

def setup_kn() -> dict:
    
    regex_instance = r".*/IJCAI2024/\w+/(?P<instance>\d+-\w+-\d+-(?P<ub>\d+)-(?P<lb>\w+)-\w+)"
    ge = True
    commands_to_check_unsat = ["clingo_plain", "wasp_python_amo", "wasp_python_kn_min", "wasp_python_kn_cmin", "wasp_plain"]
    commands_to_check_unsat += ["clingo_default_kn_ge_amo_ap"]
    commands_to_check_unsat += ["wasp_default_kn_ge_amo_ap"]
    commands_to_check_unsat += ["wasp_min_kn_ge_eo", "wasp_default_kn_ge_eo", "clingo_default_kn_ge_amo_nap_imp","clingo_default_kn_ge_amo_ap_sl", "wasp_default_kn_ge_amo_nap", "clingo_min_kn_ge_amo_nap"]
    
    base_command="clingo_default_kn_ge_amo_ap"
    
    commands_to_check_sat = ["wasp_default_kn_ge_amo_ap"]
    commands_to_check_sat += ["clingo_default_kn_ge_amo_ap"]
    commands_to_check_sat += ["wasp_min_kn_ge_eo", "wasp_default_kn_ge_eo", "clingo_default_kn_ge_amo_nap_imp", "clingo_default_kn_ge_amo_ap_sl", "wasp_default_kn_ge_amo_nap", "clingo_min_kn_ge_amo_nap"]
    
    specific_map = {
        "regex_instance": regex_instance,
        "ge": ge,
        "commands_to_check_unsat": commands_to_check_unsat,
        "base_command": base_command,
        "commands_to_check_sat": commands_to_check_sat
    }
    return specific_map

def setup_gc() -> dict:
    regex_instance = r".*/IJCAI2024/\w+/(?P<instance>\w+-\w+-\w+-\d+_(?P<lb>\d+).*)"

    ge = True
    commands_to_check_unsat = ["clingo_gc_amo", "wasp_gc_amo", "wasp_default_gc_ge_amo"]
   
    base_command="clingo_gc_amo"
    
    commands_to_check_sat = ["wasp_default_gc_ge_amo"]

    specific_map = {
        "regex_instance": regex_instance,
        "ge": ge,
        "commands_to_check_unsat": commands_to_check_unsat,
        "base_command": base_command,
        "commands_to_check_sat": commands_to_check_sat
    }

    return specific_map


def setup(problem: str) -> dict:
    
    if problem == "kn":
        specifc_map =  setup_kn()
    elif problem == "gc":
        specifc_map = setup_gc()
    else:
        assert False

    map_inconsistent_regex = {
        "clingo_plain" : r"INCONSISTENT",
        "wasp_plain" : r"INCOHERENT",
        "clingo_default_kn_ge_amo_ap": r"found 0 models"
    }

    # Knapsack
    map_inconsistent_regex["wasp_python_amo"] = map_inconsistent_regex["wasp_plain"]
    map_inconsistent_regex["wasp_python_kn_min"] = map_inconsistent_regex["wasp_plain"]
    map_inconsistent_regex["wasp_python_kn_cmin"] = map_inconsistent_regex["wasp_plain"]
    map_inconsistent_regex["wasp_default_kn_ge_amo_ap"] = map_inconsistent_regex["clingo_default_kn_ge_amo_ap"]
    map_inconsistent_regex["clingo_default_kn_ge_amo_ap_sl"] = map_inconsistent_regex["clingo_default_kn_ge_amo_ap"]
    map_inconsistent_regex["wasp_default_kn_ge_amo_nap"] = map_inconsistent_regex["clingo_default_kn_ge_amo_ap"]
    map_inconsistent_regex["clingo_default_kn_ge_amo_nap"] = map_inconsistent_regex["clingo_default_kn_ge_amo_ap"]
    map_inconsistent_regex["clingo_min_kn_ge_amo_nap"] = map_inconsistent_regex["clingo_default_kn_ge_amo_ap"]
    map_inconsistent_regex["clingo_default_kn_ge_amo_nap_imp"] = map_inconsistent_regex["clingo_default_kn_ge_amo_ap"]
    map_inconsistent_regex["wasp_default_kn_ge_eo"] = map_inconsistent_regex["clingo_default_kn_ge_amo_ap"]
    map_inconsistent_regex["wasp_min_kn_ge_eo"] = map_inconsistent_regex["clingo_default_kn_ge_amo_ap"]

    # Graph Colouring
    map_inconsistent_regex["wasp_gc_amo"] = map_inconsistent_regex["wasp_plain"]
    map_inconsistent_regex["clingo_gc_amo"] = map_inconsistent_regex["clingo_plain"]
    map_inconsistent_regex["wasp_default_gc_ge_amo"] = map_inconsistent_regex["clingo_default_kn_ge_amo_ap"]
    
    map_regex_satisfability = {
        "wasp_default_kn_ge_amo_ap" : r"mps:\s(?P<mps>\d+)",
    }

    # Knapsack
    map_regex_satisfability["clingo_default_kn_ge_amo_ap"] = map_regex_satisfability["wasp_default_kn_ge_amo_ap"]
    map_regex_satisfability["clingo_default_kn_ge_amo_ap_sl"] = map_regex_satisfability["wasp_default_kn_ge_amo_ap"]
    map_regex_satisfability["wasp_default_kn_ge_amo_nap"] = map_regex_satisfability["wasp_default_kn_ge_amo_ap"]
    map_regex_satisfability["clingo_default_kn_ge_amo_nap"] = map_regex_satisfability["wasp_default_kn_ge_amo_ap"]
    map_regex_satisfability["clingo_min_kn_ge_amo_nap"] = map_regex_satisfability["wasp_default_kn_ge_amo_ap"]
    map_regex_satisfability["clingo_default_kn_ge_amo_nap_imp"] = map_regex_satisfability["wasp_default_kn_ge_amo_ap"]
    map_regex_satisfability["wasp_default_kn_ge_eo"] = map_regex_satisfability["wasp_default_kn_ge_amo_ap"]
    map_regex_satisfability["wasp_min_kn_ge_eo"] = map_regex_satisfability["wasp_default_kn_ge_amo_ap"]
  
    # Graph Colouring
    map_regex_satisfability["wasp_default_gc_ge_amo"] = map_regex_satisfability["wasp_default_kn_ge_amo_ap"]


    common_map = {
        "map_inconsistent_regex": map_inconsistent_regex,
        "map_regex_satisfability": map_regex_satisfability,
    }
    return common_map | specifc_map

def main():
    # Parse the XML file
    param = init_run(sys.argv)
    tree = ET.parse(param.get("f"))
    root = tree.getroot()

    # p = "kn"
    p = "gc"
    map = setup(problem=p)

    # checking for unsatness
    print("checking correctness for unsatness..")
    res_unsat = check_unsatcorrectness(root=root, regex_instance=map["regex_instance"], 
                                       commands_to_check=map["commands_to_check_unsat"], 
                                    map_inconsistent_regex=map["map_inconsistent_regex"], base_command=map["base_command"])
    print("Success unsatness :)") if  res_unsat else print("Unsuccess unsatness :(")
  
    # checking for satisfability
    print("checking correctness for satisfability..")
    res_sat = check_satisfability(root=root, regex_instance=map["regex_instance"], 
                                commands_to_check=map["commands_to_check_sat"], 
                                map_regex_satisfability=map["map_regex_satisfability"], ge=map["ge"])
    print("Success satisfability :)") if  res_sat else print("Unsuccess satisfability :(")

if __name__ == "__main__":
    main()

