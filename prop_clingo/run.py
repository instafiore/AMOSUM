#!/home/s.fiorentino/.local/bin/python
import sys
import os
# adding the root path
sys.path.append(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
from prop_clingo.propagator_clingo_py.runner_clingo import RunnerClingoPython
from prop_clingo.propagator_clingo_c.runner_clingo import RunnerClingoC
from utility import *

'''
This is the entry file to run the AMO SUM propagator(s) CLINGO
'''

def main():
    
    DEFAULT_LANG = "py"
    param = init_param(sys.argv)
    
    language = param.get("lang", DEFAULT_LANG)
    runner : RunnerClingoC | RunnerClingoPython
    if language == "py":
        runner = RunnerClingoPython(parameters=param)
    elif language == "c":
        runner = RunnerClingoC(parameters=param)
    else:
        assert False
    runner.run()

if __name__ == '__main__':
    main()