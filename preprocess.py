import sys
from prop_wasp.wasp import *


starts_atoms = False
ignore_all = False
ids = set()

for line in sys.stdin:
    if ignore_all:
        print(line.strip())
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
                ids.add(terms[-1])
                # print(l[1])
                            
        print(line.strip())

print(ids)