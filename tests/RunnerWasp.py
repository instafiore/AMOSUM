import json
import subprocess
from typing import Dict, List
import re
import sys
import os
import ast
from  utility import *
import utility
sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..')))
import settings

class RunnerWasp:
    '''
    This class is meant to run experiments on the AMO sum propagator(s)
    '''

    # the solver that you are using
    SOLVER = "wasp_python"
    # SOLVER = "wasp"

    # whether or not running a test for the correctness
    CHECKING_CORRECTNESS = False
    # whether to print or not that an answer set is not in another answerset
    PRINT_ANS_AGGR_NOT_SUBSET_OF_ANS_GROUP = False
    PRINT_ANS_GROUP_NOT_SUBSET_OF_ANS_AGGR = False
    # whether to print or not the answerset aggr with its mps in the checking correctness 
    PRINT_ANS_AGGR = False
    # whether to print or not the answerset group with its mps in the checking correctness 
    PRINT_ANS_GROUP = False
    
    # whether or not printing at the end just when the run failed (correctness)
    PRINT_JUST_ERRORS = False
    
    # whether printing or not the run command
    PRINT_RUN = False

    # whether printing the output of the solver
    PRINT_OUTPUT_SOLVER = False

    # whether printing the output of the solver
    PRINT_ANSWERSETS = True

    # whether printing the error output of the solver
    PRINT_ERROR_SOLVER = True

    # REGEXs
    KNAPSACK_REGEX = r'^(knapsack|kn|ks)$'
    GRAPH_COLOURING_REGEX = r'^(graph_colouring|gc)$'
    SIMPLE_TEST_REGEX = r'^(simple_tests?|st)$'
    VALID_REGEX = [KNAPSACK_REGEX, GRAPH_COLOURING_REGEX, SIMPLE_TEST_REGEX]
    REGEX_WEIGHT_ATOM_KN = r"object\((\d+),(\d+),(\d+)\)\."
    REGEX_WEIGHT_ATOM_GC = r"colour_weight\((\w+),(\d+)\).\."


    # PROBLEMS
    KNAPSACK = "knapsack"
    GRAPH_COLOURING = "graph_colouring"
    SIMPLE_TEST = "simple_tests"

    # silent mode
    SILENT = ""
    # SILENT = "--silent=2"

    TIMEOUT = 20
    TIMEOUT_LIGHT = 10

    

    def __init__(self, parameters: Dict[str,str]) -> None:

        self.param = parameters

        set_debug(self.param.get("d",""))
        
        valid_enc_type = settings.MAP_ENC_ENCODING_FILES.keys()
        valid_prop_type = settings.MAP_PROPAGATOR.keys()

        if not "problem" in self.param:
            raise Exception(f"No problem inserted. Feasible key:'problem'")

        self.problem = self.param["problem"]
        
        if not any([not re.match(r, self.problem) is None for r in RunnerWasp.VALID_REGEX]) :
            raise Exception(f"Invalid problem inserted! Valid Regex: {str(RunnerWasp.VALID_REGEX)}")
        
        if re.match(RunnerWasp.KNAPSACK_REGEX,self.problem):
            self.problem = RunnerWasp.KNAPSACK
            # object\((\d+),(\d+),(\d+)\)\.
            self.weight_parm_id = 3
        elif re.match(RunnerWasp.GRAPH_COLOURING_REGEX,self.problem):
            # colour_weight\((\w+),(\d+)\).\.
            self.problem = RunnerWasp.GRAPH_COLOURING
            self.weight_parm_id = 2
        elif re.match(RunnerWasp.SIMPLE_TEST_REGEX,self.problem):
            self.problem = RunnerWasp.SIMPLE_TEST
        else:
            assert False

        self.light = self.param["l"] if "l" in self.param else None
        self.num_models = self.param.get("n","")
        # if there is the checking correctness mode all answersets have to be returned
        RunnerWasp.CHECKING_CORRECTNESS = self.param.get("cc", RunnerWasp.CHECKING_CORRECTNESS)
        self.num_models = "0" if RunnerWasp.CHECKING_CORRECTNESS else self.num_models

        self.n0 = f"-n{self.num_models}" if self.num_models != "" else ""
        # print(f"num_models {num_models}")
        # print(f"self.n0 {self.n0}")
        self.ass = self.param["ass"] if "ass" in self.param else ""

        # print("utility.REGEX_ASSUMPTIONS", utility.REGEX_ASSUMPTIONS)
        if self.ass != "" and re.match(utility.REGEX_ASSUMPTIONS, self.ass) is None:
            raise  Exception(f"No valid assumptions defined, feasible values are: {utility.VALID_VALUES_ASS}")


        # utility.debug("assumptions:", self.ass)
       
        self.enc_type = self.param.get("enc_type", None)
        self.prop_type = self.param.get("prop_type", None)
        self.id = self.param.get("id",0)
        
        
        self.weights = self.param["w"] if "w" in self.param else None

        self.lb = self.param["lb"] if "lb" in self.param else None
        self.ub = self.param["ub"] if "ub" in self.param else None
        self.seed = self.param.get("seed","")

        lb_gc_constraints = self.problem == RunnerWasp.GRAPH_COLOURING and (not self.lb or not self.weights)and re.match("ge",self.enc_type)
        up_gc_constraint = self.problem == RunnerWasp.GRAPH_COLOURING and (not self.ub or not self.weights) and re.match("le",self.enc_type)
        
        if lb_gc_constraints or up_gc_constraint:
            raise Exception(f"Constraint of the graph colouring didn't meet, possible problems:\n\tYou did not define a proper bound for the graph colouring problem! ge -> lb, le -> up.\n\tYou did not require to put the 'weights' into the code")

        # whether or not the number of tests is specified, 
        # if it is not it will run all the instances for the given problem
        number_of_tests = self.param.get("nt",0)
    
        head = ""
        if number_of_tests != -1:
            head = f"| head -n {number_of_tests}"

        instances = "instances"

        if self.light:
            instances = "instances_light"


        # timeout in minutes
        self.timeout_m = RunnerWasp.TIMEOUT
        if self.light:
            self.timeout_m = RunnerWasp.TIMEOUT_LIGHT
        
        self.location = f"{settings.BENCHMARKS_LOCATION}/{self.problem}"

        self.str_weights = f"{self.location}/{settings.WEIGHTS}.asp" if self.weights else ""
        self.str_lb = f"{self.location}/lb.asp" if self.lb else ""
        self.str_ub = f"{self.location}/ub.asp" if self.ub else ""

        if self.light:
            self.location_instance = f"{settings.BENCHMARKS_LOCATION}/{self.problem}/instances_light"
        else:
            self.location_instance = f"{settings.BENCHMARKS_LOCATION}/{self.problem}/instances"

        self.specific_instance = self.param["i"]  if "i" in self.param else None

        if (self.enc_type is None and self.specific_instance is None) or (self.enc_type and not self.enc_type in valid_enc_type):
            raise Exception(f"No valid encoding type key defined, feasible key:'enc_type', feasible values are: {str(valid_enc_type)}")
        
        if (self.enc_type is None and self.specific_instance is None) or (self.enc_type and not self.enc_type in valid_enc_type):
            raise Exception(f"No valid problem type key defined, feasible key:'prob_type', feasible values are: {str(valid_prop_type)}")


        if self.specific_instance and self.problem == RunnerWasp.SIMPLE_TEST:
            self.location_instance = f"{settings.BENCHMARKS_LOCATION}/{self.problem}"
    
        if self.specific_instance is None:
            self.create_instances(instances=instances, head=head)
        else:
            self.instances = False
        
        self.compare =  self.param.get("compare", False)

        self.exp = self.param.get("exp",False)

        self.enc = self.param.get("enc", False)

        if self.enc and not re.match(settings.FILE_REGEX, self.enc):
            raise Exception(f"Invalid file path for enc, {self.enc} is not a path !")
        
        self.timestamp = subprocess.run('date +"%Y-%m-%d.%H.%M.%S"', shell=True, capture_output=True, text=True).stdout
        self.timestamp = self.timestamp.strip()

        
    def create_instances(self, instances, head):
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

    def get_number_answersets(self, answer_sets):
        # for a in answer_sets:
        #     if len(a) == 0:
        #         return 0
        #     break
        return len(answer_sets)


    def create_weights_atom_regexes(self, instance):

        weights_file = None 
        atom_re = None
        if self.problem == RunnerWasp.KNAPSACK:
            weights_file = f"{self.location_instance}/{instance}.asp" if not self.exp else instance
            regex_weights = RunnerWasp.REGEX_WEIGHT_ATOM_KN
            atom_re = r"in_knapsack\((\w+),(\w+?)\)"
        elif self.problem == RunnerWasp.GRAPH_COLOURING:
            if self.exp:
                # TODO: update this for expertiments with clingo 
                assert False
            weights_file = self.str_weights
            regex_weights = RunnerWasp.REGEX_WEIGHT_ATOM_GC
            atom_re= r"col\((\w+),(\w+?)\)"
        


        maps_weights = self.create_maps_weights(weights_file, regex_weights=regex_weights) \
            if not atom_re is None and not weights_file is None else None
        

        return (maps_weights, atom_re)

    def check_correctness(self, answer_sets_aggr, answer_sets_group, instance, time_group, time_aggr):
   
        maps_weights, atom_re = self.create_weights_atom_regexes(instance=instance)

        correct = True

        if RunnerWasp.PRINT_ANS_GROUP:
            print("Answer_sets_group")
            self.print_ans(answer_sets=answer_sets_group, maps_weight=maps_weights, atom_re=atom_re, time=time_aggr)
        if RunnerWasp.PRINT_ANS_AGGR:
            print("Answer_sets_aggr")
            self.print_ans(answer_sets=answer_sets_aggr, maps_weight=maps_weights, atom_re=atom_re, time=time_group)

        for ans_1 in answer_sets_aggr:
            if not ans_1 in answer_sets_group:
                if RunnerWasp.PRINT_ANS_AGGR_NOT_SUBSET_OF_ANS_GROUP:
                    print(f"The answer set {ans_1} is not it answer_sets_group. MPS: {self.compute_mps(ans=ans_1, maps_weights=maps_weights, atom_re=atom_re)}")
                correct = False

        for ans_2 in answer_sets_group:
            if not ans_2 in answer_sets_aggr:
                if RunnerWasp.PRINT_ANS_GROUP_NOT_SUBSET_OF_ANS_AGGR:
                    print(f"The answer set {ans_2} is not it answer_sets_aggr. MPS: {self.compute_mps(ans=ans_2, maps_weights=maps_weights, atom_re=atom_re)}")
                correct = False
    

        return correct

    def print_ans(self,answer_sets, time, atom_re, maps_weight):

        print(f"Time: {time}, found {len(answer_sets)} models:")
        for i, model in enumerate(answer_sets):
            mps_str = f" mps: {self.compute_mps(model, maps_weights=maps_weight, atom_re=atom_re)}"\
                  if not maps_weight is None and not atom_re is None else ""
            print(f"Model {i+1}: {model} {mps_str}")
            
    def compute_mps(self, ans, maps_weights, atom_re):
        mps = 0
        for atom in ans:
            match = re.match(atom_re, atom)
            key = match.group(1)
            mult = int(match.group(2)) if self.problem == RunnerWasp.KNAPSACK else 1
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
    
    def get_regex_query_atom_answerset(self):
        
        regex_query : str
        if self.problem == RunnerWasp.GRAPH_COLOURING:
            regex_query = r"(?<=[\s,{])col\(\w+,\w+?\)" 
        elif self.problem == RunnerWasp.KNAPSACK:
            regex_query = r"(?<=[\s,{])in_knapsack\(\w+,\w+?\)" 
        elif self.problem == RunnerWasp.SIMPLE_TEST:
            regex_query = r"\w+[\(]?[\w,]*[\)]?" 
        else:
            assert False
        
        return regex_query
    
    def run(self):
        
        if self.instances:
            for instance in self.instances:
                self.run_classical_test(instance=instance)
        elif self.compare and self.specific_instance:
            self.run_classical_test(instance=self.specific_instance)
        elif self.specific_instance:
            enc_aggr = self.param.get("enc_aggr", False)
            encoding = settings.MAP_ENC_ENCODING_FILES[self.enc_type][0 if enc_aggr else 1] \
                if self.enc_type else None
            encoding = self.enc if self.enc else encoding
            answersets, time = self.run_instance(self.specific_instance, encoding=encoding, group_type = not enc_aggr)
            maps_weights, atom_re = self.create_weights_atom_regexes(instance=self.specific_instance)
            self.print_ans(answer_sets=answersets, time=time, atom_re=atom_re, maps_weight=maps_weights)
        else:
            assert False


    def run_instance(self, instance, encoding = None, group_type = True):

        # defining the lower bound(s)
        self.create_bound(instance=instance, ub=False)
    
        # defining the upper bound(s)
        self.create_bound(instance=instance, ub=True)
        
        location_encoding = f"{self.location}/{encoding}.asp" if encoding else ""
        location_encoding = encoding if self.enc else  location_encoding
        location_encoding = f"{self.location}/{encoding}.asp" if self.enc and not self.exp else location_encoding
        location_instance = f"{self.location_instance}/{instance}.asp" if not self.exp else instance

        print(f"location_encoding {location_encoding}")
        print(f"location_instance {location_instance}")

        timeout_str = f"timeout {self.timeout_m}m" if not self.exp else ""

        run = f"clingo \
            {location_instance} \
            {self.str_weights} \
            {location_encoding} \
            {self.str_lb} \
            {self.str_ub} \
            --output=smodels | {timeout_str} time -p {RunnerWasp.SOLVER} {RunnerWasp.SILENT} {self.n0} "
        
        id_param = f"-id {self.id}"
        ass_param = f" -ass {self.ass}" if self.ass != "" else ""
        write_stats_reason = f" -write_stats_reason" if "write_stats_reason" in self.param else ""
        minimization = self.param.get("min_r", Minimize.NO_MINIMIZATION.value)
        debug = f" -d {self.param.get('d','')}" if self.param.get("d","") != "" else ""


        possible_values_minimize = [member.value for member in Minimize]
        if not minimization in possible_values_minimize:
            raise Exception(f"The inserted value {minimization} is not valid, possible values are {possible_values_minimize}")
        min_param = f" -min_r {minimization}"
        if group_type and self.prop_type:
            propagator = settings.MAP_PROPAGATOR[self.prop_type]
            run += f"--interpreter=python \
            --script-directory={settings.PROPAGATOR_DIR_LOCATION_WASP} \
            --plugins-file=\"{propagator} {id_param}{min_param}{ass_param}{write_stats_reason}{debug}\""
  
        if self.PRINT_RUN:
            print(f"\run:\n{run}")
            
        # running test
        run_process = subprocess.run(run, shell=True, capture_output=True)

        output = run_process.stdout.decode()
        error = run_process.stderr.decode()
        output_error = output + error

        lines_output = output.splitlines() 
        lines_error = error.splitlines() 
        
        output = output.strip()
        if RunnerWasp.PRINT_OUTPUT_SOLVER and output != "" :
            print(self.param)
            print(output)

        avoiding_time_information_regex = r"(real \d+\.\d+|user \d+\.\d+|sys \d+\.\d+)"
        error = re.sub(avoiding_time_information_regex, "", error, count=0, flags=0).strip()
        if RunnerWasp.PRINT_ERROR_SOLVER and error != "":
            print(self.param, file=sys.stderr)
            print(error, file=sys.stderr)

        regex_real = r"^real\s(\d+\.\d+)"
        # regex for the answer set of a given problem
        regex_answer_set = r"^\{(.+)\}"
        
        # regex for the interested atoms of a given problem
        regex_query = self.get_regex_query_atom_answerset()

        answer_sets = []

        for line in lines_output:
            if not re.search(regex_answer_set, line) is None:
                answer_set_str = re.search(regex_answer_set, line).group(1)
                answer_set = set(re.findall(regex_query, answer_set_str))
                answer_sets.append(set(answer_set))
        
        for line in lines_error:
            if not re.search(regex_real, line) is None:
                time = re.search(regex_real, line).group(1)
            elif not re.search(r"Killed: Bye!", line) is None:
                time = "timeout"

        # restoring the instance.asp file
        self.comment_bound(instance=instance, ub=False, restore=True)
        self.comment_bound(instance=instance, ub=True, restore=True)

        return answer_sets, time
    
    def run_classical_test(self, instance: str):
            
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

        output_strings = []
        for i in range(len(encodings)):
            encoding = encodings[i]
            group = (i + 1)%2 == 0
            (answer_sets, time) = self.run_instance(instance, encoding, group )
            # print(f"encoding {encoding} answer_sets {answer_sets}")
            if group:
                answer_sets_group = answer_sets
                time_group = time
            else:
                answer_sets_aggr = answer_sets
                time_aggr = time

            output_strings.append(f"[{encoding} {time} {self.get_number_answersets(answer_sets)}]")

    
        ng = self.get_number_answersets(answer_sets_group)
        na = self.get_number_answersets(answer_sets_aggr)

        if RunnerWasp.CHECKING_CORRECTNESS:
            correct = self.check_correctness(answer_sets_aggr=answer_sets_aggr, answer_sets_group=answer_sets_group, 
                                            instance=instance,
                                            time_group=time_group, time_aggr=time_aggr)
        elif ng != na:
            correct = False
        else:
            correct = True

        equal = "equal" if correct  else "not_equal" 
        final_string = f" ".join([f"{self.param} with instance: {instance}"] + output_strings)
        if not correct:
            print(f"{final_string} [NOT CORRECT]")
        elif not RunnerWasp.PRINT_JUST_ERRORS:
            print(f"{final_string} [CORRECT]")

        # printing the new line of the test
        lb_string = ""
        if self.lb:
            lb_string = "," + str(self.lb) 
        ub_string = ""
        if self.lb:
            ub_string = "," + str(self.ub) 

        if self.problem == RunnerWasp.KNAPSACK:
            groups = re.search(r"^\d+?-\w+?-\d+?-(?P<b0>\d+)-(?P<b1>\d+)-(?P<type>\w+)", instance)
            b0 = groups.group("b0")
            b1 = groups.group("b1")
            type = groups.group("type")
            new_line = f"{number},{problem},{size},{time_aggr},{time_group},{na},{ng},{equal},{b0},{b1},{type}"
        else:
            new_line = f"{number},{problem},{size},{time_aggr},{time_group},{na},{ng},{equal}{lb_string}{ub_string}"

        if self.param.get("write_res",False):
            subprocess.run(f"echo '{new_line}' >> {settings.RESULTS_TESTS_LOCATION}/{self.problem}.{self.timestamp}.res ", shell=True, capture_output=True)


    def comment_bound(self, instance, ub = False, restore=False):
        b_str = "ub" if ub else "lb" 
        b = self.ub if ub else self.lb

        comment_r = "%" if restore else ""
        comment_w = "" if restore else "%"
        pattern = re.compile(rf"{comment_r}({b_str}\(\d+,\d+\))\.")

        instace_path = f"{self.location_instance}/{instance}.asp" if not self.exp else instance

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
            if re.match(r"\[\((\d+),(\d+)\)(,\((\d+),(\d+)\))*\]",b):
                b_list = ast.literal_eval(b)
                subprocess.run(f"echo '' > {settings.BENCHMARKS_LOCATION}/{self.problem}/{b_str}.asp", shell=True)
                for b, bi in b_list:
                    subprocess.run(f"echo '{b_str}({b},{bi}).' >> {settings.BENCHMARKS_LOCATION}/{self.problem}/{b_str}.asp", shell=True)
            elif re.match(r"\d+", b):
                subprocess.run(f"echo '{b_str}({b},{self.id}).' > {settings.BENCHMARKS_LOCATION}/{self.problem}/{b_str}.asp", shell=True)
            else:
                raise Exception(f"invalid {b_str} insert, it has to be two integers: value id ; or a list of pairs of integers(json like)") 
            
            self.comment_bound(instance=instance, ub=ub, restore=False)
       

    