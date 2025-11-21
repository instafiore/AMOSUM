#!/home/mazzotta/Experiments/solvers/bin_fiorentino/amosum_env/bin/python
import sys
import os
import re
sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..')))
from amowasp.propagator_wasp_py.runner_wasp  import RunnerWasp
from utility import *
'''
This is the entry file to run the AMO SUM propagator(s) WASP
'''

def main():
    
    param = init_param(sys.argv)
    runner = RunnerWasp(parameters=param)
    exitCode = runner.run()
    # print(f"exit code: {exitCode}")
    exit(exitCode)


if __name__ == '__main__':
    main()