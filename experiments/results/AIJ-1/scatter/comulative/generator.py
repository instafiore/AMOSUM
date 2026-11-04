import pandas as pd
import numpy as np 
import sys
import re
data = sys.argv[1]

df = pd.read_csv(data) 

instances = df.groupby('instance')

executables = sorted(list(df["executable"].unique()))
# executables = ["wasp-plain-amo", "ijcai-wasp-amosum-amo-py"]

result = []

header = ["instance"] + [f"{executables[i]}:{i+2}" for i in range(len(executables))]

for name, instance in instances:
    row = [name]
    for executable in executables:
        if executable in instance["executable"].values:
            value = instance[instance["executable"]== executable]["real"].values[0]
            value = 1200 if value > 1200 else value
            row.append(value)
        else:
            row.append(np.nan)
    result.append(row)

pivot = pd.DataFrame(result, columns=header)

pivot.to_csv("scatter.txt")

