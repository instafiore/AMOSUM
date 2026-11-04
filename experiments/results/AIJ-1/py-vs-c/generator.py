import pandas as pd
import numpy as np 
import sys
import re
data = sys.argv[1]

df = pd.read_csv(data) 

df = df[df["solver"] == "clingo"]
# df = df[df["solved"] != "-"]


regex_excecutables = [r"ijcai", r"D-clingo.*full"]
versions = df.groupby(["version"])

for version, version_group in versions:
    dfs = []
    print("\\midrule")
    start = True
    for regex in regex_excecutables:
        dfs.append(version_group[version_group["executable"].str.contains(regex)])
    for df in dfs:
        problems = df.groupby(["problem"])
        totalPy = df[df["lang"] == "py"]
        totalPy = totalPy[totalPy["solved"] != "-"].count()
        totalC = df[df["lang"] == "c"]
        totalC = totalC[totalC["solved"] != "-"].count()
        values = []
        for problem, prob_group in problems:
            langs = prob_group.groupby(["executable", "lang"])
            value_inner = []
            for lang, lang_group in langs:
                value = lang_group[lang_group["solved"] != "-"]
                value = value["solved"]
                value = int(value.count())
                value_inner.append(value)
            value_inner.reverse()
            # print(f"{version} {lang[0]} {problem} {value_inner}")
            values.append(value_inner)
        values.append([int(totalPy["solved"]), int(totalC["solved"])])

        n = len(df['executable'].unique())
        if start:
            print(f"\\multirow{{{n}}}{{*}}{{\\rotatebox{{90}}{{{version[0]}}}}} & {df['executable'].iloc[0]} & ", end="")
            start = False
        else:
            print(f" & {df['executable'].iloc[0]} & ", end="")
        print(" & ".join([" & ".join((str(e) for e in v)) if type(v) == list else str(v) for v in values]), end = "\\\\\n")        
