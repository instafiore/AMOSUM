#!/usr/bin/python3

import re
import xml.etree.ElementTree as ET
import sys

from check_correctness.setup import REGEX_INSTANCE, get_all_commands

def check_unsatcorrectness(root: ET.Element, map_inconsistent_regex, 
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
                        
                instance = re.search(REGEX_INSTANCE, test_case.attrib["id"]).group("instance")
                
                if timeoout:
                    timeout_instances.add(instance)
                elif unsat:
                    unsat_instances.add(instance)
                

        return unsat_instances, timeout_instances, present

    unsat_instances_base, timeout_instances_base, present = compute_unsat_instances(base_command)
    if not present:
        raise Exception(f"Base command {base_command} not present")
    correct = True

    # print(f"unsat_instances_base: {unsat_instances_base}")
    # print(f"timeout_instances_base: {timeout_instances_base}")
    for command in get_all_commands(root=root):
        if command == base_command: continue 

        unsat_instances_command, timeout_instances_command, present = compute_unsat_instances(command_id=command)
        if not present:
            continue
        
        # print(f"command:{command} \nunsat_instances_command: {unsat_instances_command} \ntimeout_instances_command: {timeout_instances_command}")
       
        # one way: if unsat base then unsat command
        for instance_base in unsat_instances_base:
            if instance_base not in unsat_instances_command and \
                instance_base not in timeout_instances_command:
                print(f"instance base: {instance_base} not in command: {command}")
                correct = False
        
        # second way: if unsat command then unsat base
        for instance_command in unsat_instances_command:
            if instance_command not in unsat_instances_base and \
                instance_command not in timeout_instances_base:
                print(f"instance command {command}: {instance_command} not in base: {base_command}")
                correct = False

        
    return correct