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


def optimized_run():
    """
    Wrapper that re-runs the module with -O optimization flag.
    """
    os.execvp(sys.executable, [sys.executable, "-O", "-m", "amosum.amowasp", *sys.argv[1:]])

def run():
    
    # param = init_param(sys.argv)
    param = parse_args()

    
    runner = RunnerWasp(parameters=param)
    exitCode = runner.run()
    exit(exitCode)


if __name__ == '__main__':
    run()