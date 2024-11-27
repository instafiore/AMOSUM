import sys
from prop_wasp.wasp import *

class amosum_aggregate:
    id : str # id aggregate
    prop_type: str # prop type

    def __init__(self, id: str, prop_type: str):
        self.id = id
        self.prop_type = prop_type
    
    def __hash__(self):
        res = hash(self.id) + hash(self.prop_type)
        return hash(res)
    
    def __eq__(self, value):
        return self.id == value.id and self.prop_type == value.prop_type
    
    def __repr__(self):
        return f"id: {self.id}, prop_type: {self.prop_type}"


def preprocess_ground_program(file: str) -> dict:
    starts_atoms = False
    ignore_all = False

    result_map = dict()
    amosum_set = set()
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
                    prop_type = terms[-1]
                    id = terms[-2]
                    amosum_set.add(amosum_aggregate(id=id, prop_type=prop_type))
                    # print(l[1])
                                
            # print(line.strip())

    result_map["amosum_set"] = amosum_set
    return result_map