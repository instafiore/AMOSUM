#!/usr/bin/python3
import sys
import os
import re
sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '../../../')))
import settings
import subprocess

output = subprocess.run(f"ls instances", shell=True, capture_output=True, text=True)
output = output.stdout.splitlines()
weights = settings.WEIGHTS_GC_TESTS_LOCATION


output_weights = subprocess.run(f"cat {weights}", shell=True, capture_output=True, text=True)
output_weights = output_weights.stdout.splitlines()

max_weight = 0
for line in output_weights:
    groups = re.search(r"colour_weight\(\w+,(?P<weight>\d+)\)\.", line)
    if not groups is None:
        weight = int(groups.group("weight"))
        if max_weight < weight:
            max_weight = weight

for line in output:
    groups = re.search(r"(?P<number>\d+)-(?P<problem>\w+)-(?P<number_nodes>\d+)-(\d+)", line)
    if not groups is None:
        instance = re.search(r"(.+?)\.asp", line).group(1)
        number_nodes = int(groups.group("number_nodes"))
        lb = number_nodes*max_weight
        for per in (0.15, 0.30, 0.45, 0.60, 0.75):
            lb_p = int(lb * per)
            subprocess.run(f"cat instances/{line} >> instances_with_lb/{instance}_{lb_p}.asp", shell=True, capture_output=True, text=True)
            subprocess.run(f"echo 'lb({lb_p}).' >> instances_with_lb/{instance}_{lb_p}.asp", shell=True, capture_output=True, text=True)

        