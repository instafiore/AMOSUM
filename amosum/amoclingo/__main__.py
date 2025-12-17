#!/home/mazzotta/Experiments/solvers/bin_fiorentino/amosum_env/bin/python
import sys
import os
# adding the root path
sys.path.append(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
from amoclingo.propagator_clingo_py.runner_clingo import RunnerClingoPython
from amoclingo.propagator_clingo_c.runner_clingo import RunnerClingoC
from utility import *

'''
This is the entry file to run the AMO SUM propagator(s) CLINGO
'''
def optimized_run():
    """
    Wrapper that re-runs the module with -O optimization flag.
    """

    os.execvp(sys.executable, [sys.executable, "-O", "-m", "amosum.amoclingo", *sys.argv[1:]])


def run():
    
    DEFAULT_LANG = "c"
    param = init_param(sys.argv)
    
    language = param.get("lang", DEFAULT_LANG)
    runner : RunnerClingoC | RunnerClingoPython
    if language == "py":
        runner = RunnerClingoPython(parameters=param)
    elif language == "c":
        runner = RunnerClingoC(parameters=param)
    else:
        assert False
    exitCode = runner.run()
    exit(exitCode)

if __name__ == '__main__':
    run()