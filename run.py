#!/home/s.fiorentino/miniconda3/bin/python3
import sys
import os
import re
sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..')))
from tests.Runner  import Runner
'''
This is the entry file to run the tests on the AMO SUM propagator(s)
'''

def main():
    
    param = {}
    regex = r"^-(.+)" 
    
    i = 1
    while i < len(sys.argv):
        
        # creating the key
        key = sys.argv[i] 
        res_regex = re.match(regex, key)
        if res_regex is None:
            raise Exception("Every key has to start with a dash! Ex: -problem knapsack")
        key = res_regex.group(1)

        if i + 1 >= len(sys.argv) :
            param[key] = True
            break
        
        value = sys.argv[i+1] 
        res_regex = re.match(regex, value)
        if res_regex is None:
            i += 2
            param[key] = value
            
        else:
            i += 1
            param[key] = True

    runner = Runner(parameters=param)
    runner.run()

if __name__ == '__main__':
    main()