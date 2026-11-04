import pandas as pd
import numpy as np 
import sys
import re
data = sys.argv[1]

df = pd.read_csv(data) 
language = "c"

df = df[df["solver"] == "clingo"]

versions = df.groupby(["version"])

regex_executables = [rf"ijcai.*{language}$", rf"^A-.*{language}$", rf"^B-.*{language}$", rf"^D-.*{language}$"]
avoid_excecutables = []

# \midrule
# \multirow{6}{*}{\rotatebox{90}{\textsc{unf+pc}}} & 0	 & 	0.25	&	260	&	245	&	195	&	700\\
for version, version_group in versions:
    executables = version_group.groupby(["executable"])
    print("\\midrule")
    start = True
    cont = 0
    valid_executables = [v for v in version_group["executable"].unique() if any([True for valid in regex_executables if re.search(valid, v)])]
    for executable, exec_group in executables:
        if not any([True for valid in regex_executables if re.search(valid, executable[0])]):
            continue
        if any([True for avoid in avoid_excecutables if re.search(avoid, executable[0])]):
            continue
        cont += 1
        problems = exec_group.groupby(["problem"])
        total = exec_group[exec_group["solved"] != "-"].count()
        values = []
        for problem, prob_group in problems:
            solved = prob_group[prob_group["solved"] != "-"]
            value = solved.count()
            # print(f"{version} {executable} {problem} {value['solved']}")
            values.append(value['solved'])
        values.append(total['solved'])
        if start:
            print(f"\\multirow{{{len(valid_executables)}}}{{*}}{{\\rotatebox{{90}}{{{version[0]}}}}} & {executable[0]} & ", end="")
            start = False
        else:
            print(f" & {executable[0]} & ", end="")
        print(" & ".join([str(v) for v in values]), end = "\\\\\n")