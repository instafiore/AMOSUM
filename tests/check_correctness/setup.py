import re
import xml.etree.ElementTree as ET
import sys

REGEX_INSTANCE = r"\('(?P<folder>.*)\/(?P<instance>[^\/]+)\.asp"


def get_all_commands(root: ET.Element): 
    raise NotImplementedError()
    commands = set()
    for test_case in root.findall('.//testcase'):
        command = test_case.find("command")
        commands.add(command.attrib["id"])
    return list(commands)

def setup() -> dict:
    
    map_inconsistent_regex = {
        "clingo_plain" : r"INCONSISTENT",
        "wasp_plain" : r"INCOHERENT",
        "clingo_amosum_c_lazy_hybrid": r"found 0 models"
    }

    map_sat_regex = {
        "clingo_plain" : r"ANSWER\n(?P<answerset>.*)",
        "clingo_amosum_c_lazy_hybrid": r"Model 1: {(?P<answerset>.*)}"
    }

    map_regex_findall_answerset = {
        "clingo_plain" : r"(\w+\(?[\w,]*\)?)\.",
        "clingo_amosum_c_lazy_hybrid": r"'(\w+\(?[\w\"\-,]*\)?)'"
    }
    

    map_res = {
        "map_inconsistent_regex": map_inconsistent_regex,
        "map_sat_regex": map_sat_regex,
        "map_regex_findall_answerset": map_regex_findall_answerset,
    }
    return map_res