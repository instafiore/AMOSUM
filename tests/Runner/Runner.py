import json
import subprocess
from typing import Dict, List
import re
import settings

class Runner:
    '''
    This class is meant to run experiments on the AMO sum propagator(s)
    '''

    # the solver that you are using
    SOLVER = "wasp_python"

    # whether or not running a test for the correctness
    CHECKING_CORRECTNESS = False

    # whether printing or not the run command
    PRINT_RUN = False

    # REGEXs
    KNAPSACK_REGEX = r'^(knapsack|kn|ks)$'
    GRAPH_COLOURING_REGEX = r'^(graph_colouring|gc)$'

    # PROBLEMS
    KNAPSACK = 0
    GRAPH_COLOURING = 1

    # silent mode
    SILENT = ""
    # SILENT = "--silent=2"

    TIMEOUT = 20
    TIMEOUT_LIGHT = 5

    def __init__(self, parameters: Dict[str,str]) -> None:
        self.param = parameters

        valid_enc_type = settings.MAP_ENC_PROP.keys()
        self.problem = self.param["problem"]

        if not re.match(Runner.KNAPSACK_REGEX,self.problem) and not re.match(Runner.GRAPH_COLOURING_REGEX,self.problem):
            raise Exception(f"Invalid problem inserted!")
        
        if re.match(Runner.KNAPSACK_REGEX,self.problem):
            self.problem_number = Runner.KNAPSACK
        elif re.match(Runner.GRAPH_COLOURING_REGEX,self.problem):
            self.problem_number = Runner.GRAPH_COLOURING
        else:
            assert False

        self.lb = self.param["lb"] if "lb" in self.param else None
        self.ub = self.param["ub"] if "ub" in self.param else None
        self.light = self.param["light"] if "light" in self.param else None
        self.enc_type = self.param["enc_type"] if "enc_type" in self.param else None
        
        if self.enc_type is None or not self.enc_type in valid_enc_type:
            raise Exception(f"No valid encoding type defined, feasible values are: {str(valid_enc_type)}")


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

        if "light" in self.param and self.param["ligh"] == "y":
            instances = "instances_light"

        output = subprocess.run(f"ls {settings.BENCHMARKS_LOCATION}/{problem}/{instances} {head}", shell=True, capture_output=True, text=True)
        lines = output.stdout.splitlines()

        self.instances = []
        for line in lines:
            regex_asp_program = r"(.+?)\.asp" 
            res_regex = re.match(regex_asp_program, line)
            if not res_regex is None:
                instance = res_regex.group(1)
                instances.append(instance)

    def run_test(self, instance: str):
        print(f"RUNNING {self.param}")
    
        # defining the lower bound(s)
        if self.lb:
            self.create_lower_bound(self.lb, problem)
    
         # defining the lower bound(s)
        if self.ub:
            self.create_upper_bound(self.ub, problem)
            
      
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
                answer_sets_group = answer_sets_group
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
        if LB == "none" and type_of_problem == 1:
            groups = re.search(r"^\d+?-\w+?-\d+?-(?P<lb0>\d+)-(?P<lb1>\d+)-(?P<type>\w+)", instance)
            lb0 = groups.group("lb0")
            lb1 = groups.group("lb1")
            type = groups.group("type")
            new_line = f"{number},{problem},{size},{time_aggr},{time_group},{na},{ng},{equal},{lb0},{lb1},{type}"
        else:
            new_line = f"{number},{problem},{size},{time_aggr},{time_group},{na},{ng},{equal},{LB}"

        subprocess.run(f"echo '{new_line}' >> {settings.RESULTS_TESTS_LOCATION}/{PROBLEM}.{TIMESTAMP}.res ", shell=True, capture_output=True)



    def run(self):
        
        self.timestamp = subprocess.run('date +"%Y-%m-%d.%H.%M.%S"', shell=True, capture_output=True, text=True).stdout
        for instance in self.instances:
            self.run_test(instance=instance)

    def check_correctness(self, answer_sets_aggr, answer_sets_group)
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

        str_weights = f"{location}/{settings.WEIGHTS}.asp" if "weights" in self.param in [0] else ""
        
        str_lb = f"{location}/lb.asp" if self.lb else ""
        str_ub = f"{location}/ub.asp" if self.ub else ""


        run = f"clingo \
            {location_instance}/{instance}.asp \
            {str_weights} \
            {location}/{encoding}.asp \
            {str_lb}\
            {str_ub}\
            --output=smodels | timeout {timeout_m}m time -p {Runner.SOLVER} {Runner.SILENT} {n0} "
        
        if group_type :
            propagator = settings.MAP_ENC_PROP[self.enc_type]
            run += f"--interpreter=python \
            --script-directory={settings.PROPAGATOR_DIR_LOCATION} \
            --plugins-file=\"{propagator} 1\""

        if self.PRINT_RUN:
            print(f"RUN: {run}")
            
        # running test
        output = subprocess.run(run, shell=True, capture_output=True).stdout.decode() + \
            subprocess.run(run, shell=True, capture_output=True).stderr.decode()
        lines = output.splitlines() 

        regex_real = r"^real\s(\d+\.\d+)"
        # regex for the answer set of a given problem
        regex_answer_set = r"\{(.+)\}"
        
        # regex for the interested atoms of a given problem
        regex_query : str
        if self.problem_number == Runner.GRAPH_COLOURING:
            regex_query = r"(?<=[\s,{])col\(\w+,\w+?\)"
        elif self.problem_number == Runner.KNAPSACK:
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
       

    def create_lower_bound(self, lb, problem):
        if lb:
            if re.match(r"\[(\d+)(,\d+)*\]",LB):
                lb = json.loads(lb)
                subprocess.run(f"echo '' > {settings.BENCHMARKS_LOCATION}/{problem}/lb.asp", shell=True)
                for lbi in range(len(LB)):
                    b = LB[lbi]
                    subprocess.run(f"echo 'lb({b},{lbi}).' >> {settings.BENCHMARKS_LOCATION}/{problem}/lb.asp", shell=True)
                LB = "-".join(LB)
            elif re.match(r"\d+", LB):
                subprocess.run(f"echo 'lb({LB},0).' > {settings.BENCHMARKS_LOCATION}/{problem}/lb.asp", shell=True)
            else:
                raise Exception("invalid lb insert, it has to be an integer or a list of pairs of integers(json like)")
            
    def create_upper_bound(self, ub, problem):
        
        # defining the upper bound(s)
        if ub:
            if re.match(r"\[(\d+)(,\d+)*\]",ub):
                ub = json.loads(ub)
                subprocess.run(f"echo '' > {settings.BENCHMARKS_LOCATION}/{problem}/ub.asp", shell=True)
                for ubi in range(len(LB)):
                    b = ub[ubi]
                    subprocess.run(f"echo 'lb({b},{ubi}).' >> {settings.BENCHMARKS_LOCATION}/{problem}/lb.asp", shell=True)
                LB = "-".join(LB)
            elif re.match(r"\d+", ub):
                subprocess.run(f"echo 'lb({ub},0).' > {settings.BENCHMARKS_LOCATION}/{problem}/ub.asp", shell=True)
            else:
                raise Exception("invalid lb insert, it has to be an integer or a list of pairs of integers(json like)")
            
