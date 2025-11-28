import sys
from amosum.amowasp.propagator_wasp_py.wasp import *
from settings import *

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
    result_map["amosum_mapweights"] = dict()
    amosum_set = [] 
    for line in file.splitlines():
        if ignore_all:
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
                if l[1].startswith(PREDICATE_AUX):
                    terms = getTerms(PREDICATE_AUX, l[1])
                    id = terms[0]
                    prop_type = terms[1]
                    amosum_set.append(amosum_aggregate(id=id, prop_type=prop_type))
                elif l[1].startswith(PREDICATE_GROUP):
                    terms = getTerms(PREDICATE_GROUP, l[1])
                    name = terms[0]
                    sign = terms[1].replace("\"","")
                    weight = int(terms[2])
                    id = terms[3]
                    aggrid = terms[4]
                    result_map["amosum_mapweights"].setdefault(aggrid, dict())
                    # result_map["amosum_mapweights"][aggrid][name] = {"sign": sign, "weight": weight, "id": id}
                    result_map["amosum_mapweights"][name] = {"sign": sign, "weight": weight, "id": id}


    result_map["amosum_set"] = amosum_set
    
    return result_map