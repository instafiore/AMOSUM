import random
import pandas as pd
import numpy as np
import sys
import re

data = sys.argv[1]
df = pd.read_csv(data)

# \addplot [mark size=1pt, color=blue, mark=triangle*] [unbounded coords=jump] table[col sep=comma, y index=1] {./cactus-amo.csv}; 
# \addlegendentry{\textsc{clingo}}

colors = [
    "blue",      # Standard blue
    "red",       # Standard red
    "green",     # Standard green
    "orange",    # A vibrant orange
    "purple"     # A distinct purple
]

marks = [
    "triangle*",  # Filled triangle
    "square*",    # Filled square
    "o",          # Circle (empty, distinct from filled shapes)
    "diamond*",   # Filled diamond
    "star"        # Star (empty, distinct from filled shapes)
]

valid_columns = [r"D-clingo-amosum-\w+-py-hybrid", r"ijcai-clingo-amosum-\w+-py", r"D-.*c(-full)?$", r"plain", r"ijcai.*-c$", r"ijca.*wasp", r"A-clingo-.*c$", r"A-wasp-.*py$", r"B-clingo-.*c$", r"B-wasp-.*py$", r"D-wasp-.*py$"]
tuples = list([(c, m) for c in colors for m in marks])
random.shuffle(tuples)
n = len(df.columns)
i = 1
while i < n:
    color, mark = tuples[i % len(tuples)]
    df_c = pd.DataFrame(data=df.iloc[:, i].values, columns=[df.columns[i]])
    column_name = df.columns[i]
    if not any(re.search(pattern, column_name) for pattern in valid_columns):
        i += 1
        continue
    f = data.replace("csv", "txt")
    print(f"\\addplot [mark size=2pt, color={color}, mark={mark}] [unbounded coords=jump] table[col sep=comma, y index={i}] {{./{f}}};")
    print(f"\\addlegendentry{{{column_name}}}")
    i += 1

