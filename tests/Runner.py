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
    # whether to print or not that an answer set is not in another answerset
    PRINT_NOT_SUBSET = False
    # whether to print or not the answerset aggr with its mps in the checking correctness 
    PRINT_ANS_AGGR = False
    # whether to print or not the answerset group with its mps in the checking correctness 
    PRINT_ANS_GROUP = False

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
    REGEX_WEIGHT_ATOM_KN = r"object\((\d+),(\d+),(\d+)\)\."
    REGEX_WEIGHT_ATOM_GC = r"colour_weight\((\w+),(\d+)\).\."

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
            # object\((\d+),(\d+),(\d+)\)\.
            self.weight_parm_id = 3
        elif re.match(Runner.GRAPH_COLOURING_REGEX,self.problem):
            self.problem_number = Runner.GRAPH_COLOURING_CODE
            # colour_weight\((\w+),(\d+)\).\.
            self.problem = Runner.GRAPH_COLOURING
            self.weight_parm_id = 2

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


        self.n0 = "-n0" if Runner.CHECKING_CORRECTNESS else "" 
        # timeout in minutes
        self.timeout_m = Runner.TIMEOUT
        if self.light:
            self.timeout_m = Runner.TIMEOUT_LIGHT
        
        self.location = f"{settings.BENCHMARKS_LOCATION}/{self.problem}"

        self.str_weights = f"{self.location}/{settings.WEIGHTS}.asp" if self.weights else ""
        self.str_lb = f"{self.location}/lb.asp" if self.lb else ""
        self.str_ub = f"{self.location}/ub.asp" if self.ub else ""


        if self.light:
            self.location_instance = f"{settings.BENCHMARKS_LOCATION}/{self.problem}/instances_light"
        else:
            self.location_instance = f"{settings.BENCHMARKS_LOCATION}/{self.problem}/instances"

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
        self.create_bound(instance=instance, ub=False)
    
        # defining the upper bound(s)
        self.create_bound(instance=instance, ub=True)
            
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

            print(f"[{encoding} {time} {len(answer_sets)}]",end="\t")

        
        ng = len(answer_sets_group)
        na = len(answer_sets_aggr)
        
        if Runner.CHECKING_CORRECTNESS:
            correct = self.check_correctness(answer_sets_aggr=answer_sets_aggr, answer_sets_group=answer_sets_group, instance=instance)
        else:
            correct = True

        equal = "equal" if correct  else "not_equal" 
        if not correct:
            print("NOT CORRECT")
        else:
            print("CORRENT", end = "")

        print()

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

        if not "write_res" in self.param or self.param["write_res"] == "y":
            subprocess.run(f"echo '{new_line}' >> {settings.RESULTS_TESTS_LOCATION}/{self.problem}.{self.timestamp}.res ", shell=True, capture_output=True)


        # restoring the instance.asp file
        self.comment_bound(instance=instance, ub=False, restore=True)
        self.comment_bound(instance=instance, ub=True, restore=True)

    def run(self):
        
       
        for instance in self.instances:
            self.run_test(instance=instance)

   

    def check_correctness(self, answer_sets_aggr, answer_sets_group, instance):
   
        if self.problem == Runner.KNAPSACK:
            weights_file = f"{self.location_instance}/{instance}.asp"
            regex_weights = Runner.REGEX_WEIGHT_ATOM_KN
            atom_re = r"in_knapsack\((\w+),(\w+?)\)"
        elif self.problem == Runner.GRAPH_COLOURING:
            weights_file = self.str_weights
            regex_weights = Runner.REGEX_WEIGHT_ATOM_GC
            atom_re= r"col\((\w+),(\w+?)\)"


        maps_weights = self.create_maps_weights(weights_file, regex_weights=regex_weights)

        correct = True

        for ans_1 in answer_sets_aggr:
            if not ans_1 in answer_sets_group:
                if Runner.PRINT_NOT_SUBSET:
                    print(f"The answer set {ans_1} is not it answer_sets_group. MPS: {self.compute_mps(ans=ans_1, maps_weights=maps_weights, atom_re=atom_re)}")
                correct = False

        for ans_2 in answer_sets_group:
            if not ans_2 in answer_sets_aggr:
                if Runner.PRINT_NOT_SUBSET:
                    print(f"The answer set {ans_2} is not it answer_sets_aggr. MPS: {self.compute_mps(ans=ans_2, maps_weights=maps_weights, atom_re=atom_re)}")
                correct = False
    
        if not correct and Runner.PRINT_ANS_GROUP:
            print("Answer_sets_group")
            self.print_ans(answer_sets=answer_sets_group, maps_weight=maps_weights, atom_re=atom_re)
        if not correct and Runner.PRINT_ANS_AGGR:
            print("Answer_sets_aggr")
            self.print_ans(answer_sets=answer_sets_aggr, maps_weight=maps_weights, atom_re=atom_re)

        return correct

    def print_ans(self,answer_sets, atom_re, maps_weight):

        for ans in answer_sets:
            print(f"ans: {ans} mps: {self.compute_mps(ans, maps_weights=maps_weight, atom_re=atom_re)}")
            
    def compute_mps(self, ans, maps_weights, atom_re):
        mps = 0
        for atom in ans:
            match = re.match(atom_re, atom)
            key = match.group(1)
            mult = int(match.group(2)) if self.problem == Runner.KNAPSACK else 1
            # print(f"atom {atom} key: {key} mult: {mult} weight: {maps_weights[key]}")
            mps += mult * maps_weights[key]    

        return mps    
        

    def create_maps_weights(self, file_weights, regex_weights):

        pattern = re.compile(regex_weights)
        maps = {}

        with open(file_weights, 'r') as file:
            for line in file:
                match = pattern.match(line.strip())
                if match:
                    key = match.group(1)
                    value = int(match.group(self.weight_parm_id))
                    maps[key] = value

        return maps


    
    def run_instance(self, instance, encoding, group_type):
        

        run = f"clingo \
            {self.location_instance}/{instance}.asp \
            {self.str_weights} \
            {self.location}/{encoding}.asp \
            {self.str_lb} \
            {self.str_ub} \
            --output=smodels | timeout {self.timeout_m}m time -p {Runner.SOLVER} {Runner.SILENT} {self.n0} "
        
        if group_type :
            propagator = settings.MAP_ENC_PROP[self.enc_type]
            run += f"--interpreter=python \
            --script-directory={settings.PROPAGATOR_DIR_LOCATION} \
            --plugins-file=\"{propagator} {self.id}\""

        if self.PRINT_RUN:
            print(f"\nRUN:\n{run}")
            
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
    


    def comment_bound(self, instance, ub = False, restore=False):
        b_str = "ub" if ub else "lb" 
        b = self.ub if ub else self.lb

        comment_r = "%" if restore else ""
        comment_w = "" if restore else "%"
        pattern = re.compile(rf"{comment_r}({b_str}\(\d+,\d+\))\.")

        instace_path = f"{self.location_instance}/{instance}.asp"

        with open(instace_path, "r") as file:
            lines = file.readlines()

        with open(instace_path, "w") as file:
            for line in lines:
                match = re.match(pattern, line)
                if match:
                    file.write(f"{comment_w}{match.group(1)}.\n")
                else:
                    file.write(line)

    def create_bound(self, instance, ub = False):
        b = self.ub if ub else self.lb
        b_str = "ub" if ub else "lb" 
        if b:
            if re.match(r"\[(\d+)(,\d+)*\]",b):
                b = json.loads(b)
                subprocess.run(f"echo '' > {settings.BENCHMARKS_LOCATION}/{self.problem}/{b_str}.asp", shell=True)
                for bi in range(len(b)):
                    b = b[bi]
                    subprocess.run(f"echo '{b_str}({b},{bi}).' >> {settings.BENCHMARKS_LOCATION}/{self.problem}/{b_str}.asp", shell=True)
                B = "-".join(b)
            elif re.match(r"\d+", b):
                subprocess.run(f"echo '{b_str}({b},{self.id}).' > {settings.BENCHMARKS_LOCATION}/{self.problem}/{b_str}.asp", shell=True)
            else:
                raise Exception(f"invalid {b_str} insert, it has to be an integer or a list of pairs of integers(json like)") 
            
            self.comment_bound(instance=instance, ub=ub, restore=False)
       

    