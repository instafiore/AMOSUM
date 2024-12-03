#!/Users/instafiore/instafiore_env/bin/python
import sys
import os
# adding the root path
sys.path.append(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
from prop_clingo.runner_clingo import RunnerClingo
from utility import *

'''
This is the entry file to run the AMO SUM propagator(s) CLINGO
'''

def main():
    
    param = init_run(sys.argv)
    # print(param)
    
    runner = RunnerClingo(parameters=param)
    runner.run()

if __name__ == '__main__':
    main()