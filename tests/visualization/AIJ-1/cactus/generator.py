import pandas as pd
import numpy as np
import sys

data = sys.argv[1]
df = pd.read_csv(data)

versions = df.groupby(["version"])

for version, version_group in versions:
    df_final = pd.DataFrame()
    executables = version_group.groupby(["executable"])
    for e, e_group in executables:
        e_group = e_group[e_group["solved"] != "-"]
        # print(group.head())
        e_group = e_group.sort_values(by=["real"])
        df_c = pd.DataFrame(data=e_group["real"].values, columns=[e[0]])
        df_final = pd.concat([df_final, df_c], axis=1)

    df_final.to_csv(f"cactus-{version[0]}.csv" ,index=True)
    df_final.to_csv(f"cactus-{version[0]}.txt" ,index=True, header=False)