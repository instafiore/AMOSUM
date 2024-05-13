import json
import subprocess
from typing import Dict, List
import re
import sys
import os
sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..')))
import settings

class Runner:
    '''
    This class is meant to run experiments on the AMO sum propagator(s)
    '''

    # the solver that you are using
    SOLVER = "wasp_python"

    # whether or not running a test for the correctness
    CHECKING_CORRECTNESS = True

    # whether printing or not the run command
    PRINT_RUN = False

    # whether printing the output of the solver
    PRINT_OUTPUT_SOLVER = False

    # whether printing the error output of the solver
    PRINT_ERROR_SOLVER = False

    # REGEXs
    KNAPSACK_REGEX = r'^(knapsack|kn|ks)$'
    GRAPH_COLOURING_REGEX = r'^(graph_colouring|gc)$'
    VALID_REGEX = [KNAPSACK_REGEX, GRAPH_COLOURING_REGEX]

    # PROBLEMS
    KNAPSACK = "knapsack"
    KNAPSACK_CODE = 0
    GRAPH_COLOURING = "graph_colouring"
    GRAPH_COLOURING_CODE = 1

    # silent mode
    SILENT = ""
    # SILENT = "--silent=2"

    TIMEOUT = 20
    TIMEOUT_LIGHT = 5

    def __init__(self, parameters: Dict[str,str]) -> None:
        self.param = parameters

        valid_enc_type = settings.MAP_ENC_PROP.keys()

        if not "problem" in self.param:
            raise Exception(f"No problem inserted Feasible key:'problem'")

        self.problem = self.param["problem"]

        
        if not re.match(Runner.KNAPSACK_REGEX,self.problem) and not re.match(Runner.GRAPH_COLOURING_REGEX,self.problem):
            raise Exception(f"Invalid problem inserted! Valid Regex: {str(Runner.VALID_REGEX)}")
        
        if re.match(Runner.KNAPSACK_REGEX,self.problem):
            self.problem = Runner.KNAPSACK
            self.problem_number = Runner.KNAPSACK_CODE
        elif re.match(Runner.GRAPH_COLOURING_REGEX,self.problem):
            self.problem_number = Runner.GRAPH_COLOURING_CODE
            self.problem = Runner.GRAPH_COLOURING
        else:
            assert False

        self.light = self.param["l"] if "l" in self.param else None
       
        self.enc_type = self.param["enc_type"] if "enc_type" in self.param else None
        self.id = self.param["id"] if "id" in self.param else "0"
        
        if self.enc_type is None or not self.enc_type in valid_enc_type:
            raise Exception(f"No valid encoding type key defined, feasible key:'enc_type', feasible values are: {str(valid_enc_type)}")

        self.weights = self.param["w"] if "w" in self.param else None

        self.lb = self.param["lb"] if "lb" in self.param else None
        self.ub = self.param["ub"] if "ub" in self.param else None

        lb_gc_constraints = self.problem == Runner.GRAPH_COLOURING and (not self.lb or not self.weights)and re.match("ge",self.enc_type)
        up_gc_constraint = self.problem == Runner.GRAPH_COLOURING and (not self.ub or not self.weights) and re.match("le",self.enc_type)
        
        if lb_gc_constraints or up_gc_constraint:
            raise Exception(f"Constraint of the graph colouring didn't meet, possible problems:\n\tYou did not define a proper bound for the graph colouring problem! ge -> lb, le -> up.\n\tYou did not require to put the 'weights' into the code")

        # whether or not the number of tests is specified, 
        # if it is not it will run all the instances for the given problem
        if "nt" in self.param:
            number_of_tests = self.param["nt"]
        else:
            number_of_tests = -1

        head = ""
        if number_of_tests != -1:
            head = f"| head -n {number_of_tests}"

        instances = "instances"

        if self.light:
            instances = "instances_light"

        getting_instance_command = f"ls {settings.BENCHMARKS_LOCATION}/{self.problem}/{instances} {head}"
        output = subprocess.run(getting_instance_command, shell=True, capture_output=True, text=True)
        if output.stderr:
            print(output.stderr)
        lines = output.stdout.splitlines()

        self.instances = []
        for line in lines:
            regex_asp_program = r"(.+?)\.asp" 
            res_regex = re.match(regex_asp_program, line)
            if not res_regex is None:
                instance = res_regex.group(1)
                self.instances.append(instance)

        self.timestamp = subprocess.run('date +"%Y-%m-%d.%H.%M.%S"', shell=True, capture_output=True, text=True).stdout
        self.timestamp = self.timestamp.strip()

    def run_test(self, instance: str):
        print(f"RUNNING {self.param} with instance: {instance}")
    
        # defining the lower bound(s)
        if self.lb:
            self.create_lower_bound()
    
         # defining the lower bound(s)
        if self.ub:
            self.create_upper_bound()
            
      
        # regex for file 
        regex_instance_file : str = r"^(?P<number>\d+)-(?P<problem>\w+)-(?P<size>\d+).*"
        groups = re.search(regex_instance_file, instance)

        number = groups.group("number")
        problem = groups.group("problem")
        size = groups.group("size")

        encodings = settings.MAP_ENC_ENCODING_FILES[self.enc_type]

        # answer sets for checking correctness
        answer_sets_aggr : List
        answer_sets_group : List
        time_group : float
        time_aggr : float

        for i in range(len(encodings)):
            encoding = encodings[i]
            group = (i + 1)%2 == 0
            (answer_sets, time) = self.run_instance(instance, encoding, group )
            if group:
                answer_sets_group = answer_sets
                time_group = time
            else:
                answer_sets_aggr = answer_sets
                time_aggr = time

            print(f"{encoding} {time} {len(answer_sets)}")

                   
        ng = len(answer_sets_group)
        na = len(answer_sets_aggr)

        if na != ng:
            correct = False
        elif Runner.CHECKING_CORRECTNESS:
            correct = self.check_correctness(answer_sets_aggr=answer_sets_aggr, answer_sets_group=answer_sets_group)
        else:
            correct = True

        equal = "equal" if correct  else "not_equal" 
        if not correct:
            print("NOT CORRECT")

        # printing the new line of the test
        lb_string = ""
        if self.lb:
            lb_string = "," + str(self.lb) 
        ub_string = ""
        if self.lb:
            ub_string = "," + str(self.ub) 

        if self.problem_number == Runner.KNAPSACK_CODE:
            groups = re.search(r"^\d+?-\w+?-\d+?-(?P<b0>\d+)-(?P<b1>\d+)-(?P<type>\w+)", instance)
            b0 = groups.group("b0")
            b1 = groups.group("b1")
            type = groups.group("type")
            new_line = f"{number},{problem},{size},{time_aggr},{time_group},{na},{ng},{equal},{b0},{b1},{type}"
        else:
            new_line = f"{number},{problem},{size},{time_aggr},{time_group},{na},{ng},{equal}{lb_string}{ub_string}"

        subprocess.run(f"echo '{new_line}' >> {settings.RESULTS_TESTS_LOCATION}/{self.problem}.{self.timestamp}.res ", shell=True, capture_output=True)



    def run(self):
        
       
        for instance in self.instances:
            self.run_test(instance=instance)

    def check_correctness(self, answer_sets_aggr, answer_sets_group):
        for ans_1 in answer_sets_aggr:
            if not ans_1 in answer_sets_group:
                return False
            
        return True

    
    def run_instance(self, instance, encoding, group_type):
        
        n0 = "-n0" if Runner.CHECKING_CORRECTNESS else "" 
        # timeout in minutes
        timeout_m = Runner.TIMEOUT
        if self.light:
            timeout_m = Runner.TIMEOUT_LIGHT
        location = f"{settings.BENCHMARKS_LOCATION}/{self.problem}"

        if self.light:
            location_instance = f"{settings.BENCHMARKS_LOCATION}/{self.problem}/instances_light"
        else:
            location_instance = f"{settings.BENCHMARKS_LOCATION}/{self.problem}/instances"

        str_weights = f"{location}/{settings.WEIGHTS}.asp" if self.weights else ""
        
        str_lb = f"{location}/lb.asp" if self.lb else ""
        str_ub = f"{location}/ub.asp" if self.ub else ""


        run = f"clingo \
            {location_instance}/{instance}.asp \
            {str_weights} \
            {location}/{encoding}.asp \
            {str_lb} \
            {str_ub} \
            --output=smodels | timeout {timeout_m}m time -p {Runner.SOLVER} {Runner.SILENT} {n0} "
        


        if group_type :
            propagator = settings.MAP_ENC_PROP[self.enc_type]
            run += f"--interpreter=python \
            --script-directory={settings.PROPAGATOR_DIR_LOCATION} \
            --plugins-file=\"{propagator} {self.id}\""

        if self.PRINT_RUN:
            print(f"RUN: {run}")
            
        # running test
        output = subprocess.run(run, shell=True, capture_output=True).stdout.decode()
            
        error = subprocess.run(run, shell=True, capture_output=True).stderr.decode()
        output_error = output + error
        lines = output_error.splitlines() 
        if Runner.PRINT_OUTPUT_SOLVER:
            print(output)

        if Runner.PRINT_ERROR_SOLVER:
            print(error)

        regex_real = r"^real\s(\d+\.\d+)"
        # regex for the answer set of a given problem
        regex_answer_set = r"\{(.+)\}"
        
        # regex for the interested atoms of a given problem
        regex_query : str
        if self.problem_number == Runner.GRAPH_COLOURING_CODE:
            regex_query = r"(?<=[\s,{])col\(\w+,\w+?\)"
        elif self.problem_number == Runner.KNAPSACK_CODE:
            regex_query = r"(?<=[\s,{])in_knapsack\(\w+,\w+?\)"

        answer_sets = []
        for line in lines:
            if not re.search(regex_real, line) is None:
                time = re.search(regex_real, line).group(1)
            elif not re.search(regex_answer_set, line) is None:
                answer_set = re.search(regex_answer_set, line).group(1)
                answer_set = set(re.findall(regex_query, answer_set))
                answer_sets.append(set(answer_set))
            elif not re.search(r"Killed: Bye!", line) is None:
                time = "timeout"

        return (answer_sets, time)
       

    def create_lower_bound(self):
        lb = self.lb
        if lb:
            if re.match(r"\[(\d+)(,\d+)*\]",lb):
                lb = json.loads(lb)
                subprocess.run(f"echo '' > {settings.BENCHMARKS_LOCATION}/{self.problem}/lb.asp", shell=True)
                for lbi in range(len(lb)):
                    b = lb[lbi]
                    subprocess.run(f"echo 'lb({b},{lbi}).' >> {settings.BENCHMARKS_LOCATION}/{self.problem}/lb.asp", shell=True)
                LB = "-".join(lb)
            elif re.match(r"\d+", lb):
                subprocess.run(f"echo 'lb({lb},{self.id}).' > {settings.BENCHMARKS_LOCATION}/{self.problem}/lb.asp", shell=True)
            else:
                raise Exception("invalid ub insert, it has to be an integer or a list of pairs of integers(json like)")
            
    def create_upper_bound(self):
        ub = self.ub
        # defining the upper bound(s)
        if ub:
            if re.match(r"\[(\d+)(,\d+)*\]",ub):
                ub = json.loads(ub)
                subprocess.run(f"echo '' > {settings.BENCHMARKS_LOCATION}/{self.problem}/ub.asp", shell=True)
                for ubi in range(len(ub)):
                    b = ub[ubi]
                    subprocess.run(f"echo 'lb({b},{ubi}).' >> {settings.BENCHMARKS_LOCATION}/{self.problem}/lb.asp", shell=True)
                LB = "-".join(ub)
            elif re.match(r"\d+", ub):
                subprocess.run(f"echo 'lb({ub},{self.id}).' > {settings.BENCHMARKS_LOCATION}/{self.problem}/ub.asp", shell=True)
            else:
                raise Exception("invalid lb insert, it has to be an integer or a list of pairs of integers(json like)")
            
