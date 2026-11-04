import pandas as pd
import numpy as np 
import sys
import re
data = sys.argv[1]

df = pd.read_csv(data) 

df = df[df["problem"] == "Knapsack"]

types = ["type1", "type2", "type3", "type4"]

for t in types:
    df_t = df[df["instance"].str.contains(t)]
    print(f"Type: {t}")
    print(df_t)
print(df)