#!/Users/instafiore/instafiore_env/bin/python3
import sys
import os
import re
sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..')))
from prop_wasp.runner_wasp  import RunnerWasp
from utility import *
'''
This is the entry file to run the AMO SUM propagator(s) WASP
'''

def main():
    
    param = init_run(sys.argv)
    runner = RunnerWasp(parameters=param)
    runner.run()

if __name__ == '__main__':
    main()