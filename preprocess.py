import sys
from prop_wasp.wasp import *


def preprocess_ground_program(file: str) -> dict:
    starts_atoms = False
    ignore_all = False
    amosum_ids = set()

    result_map = dict()
    for line in file.splitlines():
        if ignore_all:
            # print(line.strip())
            continue
        l = line.split()    
        if len(l) == 1 and l[0] == "0":
            if not starts_atoms:
                starts_atoms = True
            else:
                starts_atoms = False
                ignore_all = True
        else:
            if starts_atoms:
                assert len(l) == 2
                if l[1].startswith("group"):
                    terms = getTerms("group", l[1])
                    amosum_ids.add(terms[-1])
                    # print(l[1])
                                
            # print(line.strip())

    result_map["amosum_ids"] = amosum_ids
    return result_map