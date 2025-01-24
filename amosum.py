import json
import sys
import os
sys.path.append(os.path.dirname(os.path.abspath(__file__)))
from ast import Tuple
from typing import Callable, List
from utility import *
import prop_wasp.propagator_wasp_py.wasp as wasp
import re
import settings
from settings import *
import time
import traceback

class AmoSumPropagator:

    atomNames : dict[str: int]

    # input parameters
    param : dict[str]

    # self.aggregate id
    ID : int

    # self.N: number of atoms in the program
    # self.N: number of atoms in the self.aggregate
    N: int

    # a function from literals -> {True, False, None}
    I : SymmetricFunction

    # a function from literals -> weights
    # assuming that the self.aggregate does not contain the to literal l and !l
    weight : WeightFunction

    # a function from literals -> {True, False}
    aggregate : AggregateFunction

    # a function from literals -> self.groups
    group : GroupFunction

    # a function from self.groups -> literals U {None}
    true_group : TrueGroupFunction

    # a list of Group
    groups : List[Group]

    # literals derived at level 0
    facts : List[int]

    # reason for literals 
    reason_falses : List[int] 

    # reason for true_literals
    reason_trues : PerfectHash

    # reason
    reason : PerfectHash

    # redundant literals in reason of a literal l
    # it is a funtion lits -> 2^(lits)
    redundant_lits : PerfectHash

    # self.assumptions as a list of atom names
    assumptions : List[str]

    # global array of ordered lit per self.weight (ascending order)
    global_ord_lit : List[int]

    # strategy with which to create the minimal reason, default is the order given in input
    strategy : str

    # the last decision literal
    last_decision_lit : int

    # type of minimization, default no minimization
    minimization : str

    # sum of percentage of redaction of reason
    sum_p : int

    # count of reducted reasons
    count_p : int

    # defining the constraint type, possible values: AMO, EO
    choice_cons : str

    # defining whether the propagator is for the constraint >=  (ge) or <= (le) 
    ge : bool

    # propagate function to implement in propagator file
    propagate_phase : Callable[[Group, 'AmoSumPropagator', dict],List[int]]


    # >= (ge) constraint
    # ----------------------------
    # lower bound
    lb : int

    # max possible sum
    _mps : int 
    # ----------------------------

    # <= (ge) constraint
    # ----------------------------
    # upper bound
    ub : int

    # min possible sum
    _mps : int 
    # ----------------------------

    # derived lits
    S : List[int] = []

    # given a specific literal if it has been propagated but not already processed (onLiteralTrue called)
    to_be_propagated : PerfectHash

    # SUPPORTED SOLVERS
    WASP = 1
    CLINGO = 2

    def __init__(self, atomNames: dict[str: int], sys_parameters: List[str], propagation_phase: Callable[[Group, 'AmoSumPropagator'],  List[int]] = None, ge: bool = True, choice_cons: str = "AMO", solver = WASP) -> None:
        self.atomNames = atomNames
        self.param = sys_parameters
        self.facts : List[int] = []
        self.reason_falses : List[int] = []
        self.strategy = 'default'
        self.last_decision_lit = 0
        self.minimization = Minimize.NO_MINIMIZATION
        self.sum_p = 0
        self.count_p = 0
        self.ge = ge
        self.choice_cons = choice_cons
        self.propagate_phase = propagation_phase
        self.solver = solver
        self.count = 0
        self.S = []
        self.lazy_perc = None

    def getLiterals(self, *lits):
        param = self.param

        set_debug(param.get("d",""))

        
        
        # initializing 
        self.minimization = param.get("min_r",Minimize.NO_MINIMIZATION.value)
        self.strategy = param.get("strategy",self.strategy)
        self.N = lits[0] + 1
        self.I = SymmetricFunction(self.N)
        self.weight = WeightFunction(self.N)
        self.group = GroupFunction(self.N)
        self.propagated = SymmetricFunction(self.N)
        self.aggregate = AggregateFunction(self.N, False)
        self.to_be_propagated = PerfectHash(self.N, False)
        self.reason_trues = PerfectHash(self.N,[])
        self.reason = PerfectHash(self.N,[])
        self.redundant_lits = PerfectHash(self.N,[])
        self._mps = 0
        self.ID = param.get("id","0")
        self.groups = []
        self.assumptions = param.get("ass", False)
        self.current_sum = 0 
        self.lazy_prop_activated = param.get("lazy",False)
        self.lazy_condition = not self.lazy_prop_activated
        self.groups_literals = []
        self.lazy_perc = None


        #used to create the self.groups
        groups_raw : dict[int, List[int]] = {}

        self.lb = None
        bind = []
        negative_lit_regex = re.compile(r"^not\s+(?P<atom_name>[\w()]+)")
        bound_str = PREDICATE_LB if self.ge else PREDICATE_UB 
        bound = None
         

        # selecting the interested literals
        self.weights_names = dict()
        for a in self.atomNames:
            if  a.startswith(f'{PREDICATE_GROUP}('):
                group_literal = self.atomNames[a]
                
                terms = wasp.getTerms(PREDICATE_GROUP,a)
                # Syntax: PREDICATE_GROUP( lit_name, weight, group_id, aggregate_id)
                if len(terms) != 5 or terms[4] != self.ID:
                    continue
                
                # setting every group_literal to false
                self.groups_literals.append(not_(group_literal))
                
                lit_str = terms[0]
                atom_name = lit_str
                match = negative_lit_regex.match(lit_str)
                if match:
                    atom_name = match.group('atom_name')
                    lit =  self.atomNames[atom_name] * -1
                else:
                    lit =  self.atomNames[atom_name]
            
                sign = 1 if re.search(r"\+",terms[1]) else -1
                lit *= sign
                # updating the self.weight
                weight = int(terms[2])
                self.weight[lit] = weight
                self.weights_names[atom_name] =  weight
                # updating the self.group id
                group_id = terms[3]

                G = groups_raw.get(group_id,[])
                G.append(lit)
                groups_raw[group_id] = G

                # adding to the self.aggregate
                self.aggregate[lit] = True
                
                bind.append(lit)
                bind.append(-lit)
            
            elif a.startswith(f"{bound_str}("):
                terms = wasp.getTerms(f'{bound_str}',a)
                if (len(terms) != 2 or terms[1] != self.ID):
                    continue
                if not bound is None:
                    assert False     
                if self.ge:
                    self.lb = int(terms[0])
                else:
                    self.ub = int(terms[0])
                bound = self.lb if self.ge else self.ub

        self.bound = bound
        assert not self.bound is None

        # creating self.groups
        for group_id in groups_raw:
            
            lits_group = groups_raw[group_id]

            # ordering by self.weight
            lits_ord = [(lit, self.weight[lit]) for lit in lits_group]
            lits_ord = sorted(lits_ord, key = lambda x: x[1])
    
            ord_l = [None] * len(lits_ord)

            # it cannot become a PerfectHash since the space required would be O(self.N^2) (self.N number of literals)
            ord_i : dict[int, int] = {}

            for i in range(len(lits_ord)):
                l = lits_ord[i][0]
                ord_l[i] = l
                ord_i[l] = i
            
            # creating the self.group
            G = Group(ord_l,ord_i,group_id)
            
            # updatind the max possible sum
            self._mps = self._mps + self.weight[m_w(G, max = self.ge)]
        
            # adding the self.group to the list of self.groups
            assert G not in self.groups
            self.groups.append(G)


            # defining the function self.group
            for lit in lits_group:
                self.group[lit] = G
        
        nGroup = Group.autoincrement 
        self.true_group = TrueGroupFunction(nGroup)

        debug(f"id: {self.ID} total_weight_names: {json.dumps(self.weights_names)}", force_print=True) if self.solver == AmoSumPropagator.WASP else None

        # PREPROCESSING
        for i in range(1,len(lits)):
            l = lits[i]
            try:
                self.update_phase(l)
                self.inconsistent_at_level_0 = False
            except Exception as e:
                self.inconsistent_at_level_0 = True

        self.facts = lits[1:]

        self.last_decision_lit = 1
        self.dl = 0        

        return bind 
        

    def simplifyAtLevelZero(self, delete_lits = False):

        # INCOHERENT
        # debug(f"_mps: {self._mps}")
        error_string = f"{self._mps} < {self.lb} !!!" if self.ge else f"{self._mps} > {self.ub} !!!"
        if (self.ge and self._mps < self.lb) or (not self.ge and self._mps > self.ub) :
            debug(error_string)
            return [1]
         
        assert not self.inconsistent_at_level_0
        
        self.update_lazy_propagation()
        prop_from_facts = self.propagate_phase(None, self, self.atomNames) if self.lazy_condition else []
        
        if delete_lits:
            simplifyLiterals(self.facts, self.aggregate, self.group, max = self.ge, I = self.I)

        if self.assumptions:
            self.assumptions = convert_assparam_to_assarray(self.assumptions)

        return  create_assumptions_lits(assumptions=self.assumptions,atomNames=self.atomNames) + prop_from_facts + self.groups_literals

    def onLiteralTrue(self, lit, dl):

        if not self.is_in_aggregate(lit):
            return []

        self.updated_dl(lit, dl)  
        self.current_literal = lit      

        if self.I[lit]:
            # If lit is already true then no progation will take place
            return []

        assert not self.I[lit] == False


        try:
            (next_phase, G) = self.update_phase(lit) 
        except Exception as e:
            raise e
        
        if dl == 0:
            # this literal can be simplified given that it derives from facts
            simplifyLiterals([lit], self.aggregate, self.group, max = self.ge, I = self.I)
        

        propagated_lits = []
        # print_starting_propagation(self, lit, next_phase)
        if next_phase:
            try:
                propagated_lits = self.propagate_phase(G, self, self.atomNames)
            except Exception as e:
                raise e
        
        return propagated_lits

    def update_lazy_propagation(self):
        
        p : float
        if self.ge:
            self.mps_violated = self._mps < self.lb 
            p = (self.bound / self._mps) if self._mps != 0 else 1
        else:
            self.mps_violated = self._mps > self.ub
            p =  (self._mps / self.bound) if self.bound != 0 else 1

        self.lazy_condition = p >= self.lazy_perc
        if self.mps_violated:
            self.lazy_condition = True

    def update_phase(self, l: int) -> tuple[bool, Group]:
    
        w_p : int = 0
        w_n : int = 0
        self.I[l] = True

        tg = False
        G : Group
        self.mps_violated : bool = False
        self.count += 1
        

        amo_condition = False
        if self.aggregate[l]:
            G = self.group[l]
            self.to_be_propagated[l] = False 
            G.decrease_und()
            self.true_group[G] = l
            w_p = self.weight[m_w(G, max = self.ge)]
            w_n = self.weight[l]
            tg = True
            self.current_sum += w_n
            
        elif self.aggregate[not_(l)]:
            G = self.group[not_(l)]
            self.to_be_propagated[not_(l)] = False 
            G.decrease_und()
            new, prev = G.update(self.I, max=self.ge, update=False, assuming_und = l)
            if not_(l) == prev:
                G.set_max_min(l=new, max=self.ge)
                if self.true_group[G] is None :
                    w_n = self.weight[new]
                    w_p = self.weight[prev]

                # it has to be true because if the mps is not changed can happen that there is another literal with the same weight that is maximum 
                # and this literal can be propagated to true
                if self.choice_cons == "AMO":
                    amo_condition = True
            elif not_(l) != new:
                # there is at least one more literal that can satify the lower bound
                # so no propagation can take place
                return (False, None)
            elif self.choice_cons == "AMO":
                amo_condition = True
            else:
                return(False, None)
        else:
            return (False, None)

        self._mps = self._mps - w_p + w_n
        self.update_lazy_propagation()

        G = G if self.choice_cons == "EO" else None
        current_sum_condition = not self.ge or self.current_sum < self.bound
        next_phase = current_sum_condition and (w_p != w_n or amo_condition) and self.lazy_condition 
                
        # debug(f"[mps: {self._mps}, id: {self.ID}] iteration: {self.count} next_phase: {next_phase} self.bound / self._mps: {self.bound / self._mps} self.lazy_condition: {self.lazy_condition} {self.lazy_perc} lazy_prop_activated: {self.lazy_prop_activated}", force_print=True)
        return (next_phase,  G)
    
    def mps(self, g: Group, l: int, assumed:bool, return_literals = False):
        if assumed:
            ml_g = m_w(g, max=max)
            mw_g = self.weight[ml_g]
            assert self.true_group[g] is None
            mps = self._mps - mw_g + self.weight[l]
            return mps if not return_literals else (mps, l, ml_g)
        else:
            assert self.true_group[g] is None
            sml_g, ml_g =  g.update(self.I, update=False, max=self.ge)
            mw_g =  self.weight[ml_g]
            if ml_g != l:
                return self._mps if not return_literals else (self._mps , sml_g, ml_g)
            mps = self._mps - mw_g + self.weight[sml_g]
            return mps if not return_literals else (mps, sml_g, ml_g)
        
    def getReasonForLiteral(self, lit):
    
        R = self.reason[lit]
        rl = self.redundant_lits[lit] 
        removed = False
    
        # removing redundant lits (if any)
        if len(rl) > 0:
            removed = True
            R = remove_elements(R, rl)
            self.redundant_lits[lit] = []
        
        # print_reason(atomNames=self.atomNames, R=R, literal=lit, force_print=False)
        # for l in R:
        #     assert self.I[l] == False
        return R 

    def compute_minimal_reason(self, to_minimize: List[int]):
        '''
        Invariants (in case of cmin strategy) reason is grouped by self.group id and in each self.group the literals are sort in descending order
        '''

        if self.minimization == Minimize.NO_MINIMIZATION.value:
            return
        
        for l in to_minimize:
            g = self.group[l]
            derived_true = True
            if g is None:
                g = self.group[not_(l)]
                derived_true = False
            assert not g is None
    
            mps = self.mps(g, l, assumed = not derived_true)
            s = self.lb - mps - 1 
    
            if self.minimization == Minimize.MINIMAL.value:
                self.redundant_lits[l] = maximal_subset_sum_less_than_s_with_groups(derived_true=derived_true, literals= self.reason[l], s = s, weight= self.weight, group=self.group, head_reason=l, I=self.I, max=self.ge)
            elif self.minimization == Minimize.CARDINALITY_MINIMAL.value:
                # outdated
                increment = compute_increment_literals(literals=self.reason[l], group=self.group, weight=self.weight)
                self.redundant_lits[l]  = maximum_subset_sum_less_than_s_with_groups(literals= self.reason[l], s = s, weight = increment, group=self.group, I=self.I)            
            else:
                assert False

    def onLiteralsUndefined(self, *lits, wasp: bool = True) -> None:
        
        for i in range(1 if wasp else 0,len(lits)):
            l = lits[i]
            if not self.is_in_aggregate(l):
                continue

            self.propagated[l] = False
            
            # This has been added to handle early stop in propagation phase (clingo propagation)
            if self.I[l] is None:
                continue


            # updating interpretation
            self.I[l] = None
            self.reason[l] = []
            self.to_be_propagated[l] = False
            

            # updating max self.weight for self.group(l)
            G : Group = self.group[l] 

            if G is None:
                G = self.group[not_(l)]
                l = not_(l)

            assert not G is None

            # increasing the number of undefined literals for G
            G.increase_und()

            tg = self.true_group[G]

            # the true literal of G becomes undefined
            w_l = self.weight[l]

            if tg == l:
                self.true_group[G] = None
                self.current_sum -= w_l

            m_und = m_w(G, max = self.ge)

            '''
            if G has all literals defined
                G.set_max(l) ifself.ge else G.set_min(l)
                return

            if l was the true literal of the self.group 
    
                # updating the self.mps
                if self.weight[m_w(G,self.ge)] > ifself.ge else < self.weight[l] -> self.mps = self.mps - self.weight[l] + self.weight[m_w(G,self.ge)]
                
                # updating the max undefined
                if pos(m_w(G,self.ge)) > ifself.ge else < pos(l) -> G.set_max(l) ifself.ge else G.set_min(l)

            else
            
                m_weight = self.weight[m_w(G,self.ge)]
                if self.weight[l] >= ifself.ge else <= m_weight and pos(l) > ifself.ge else < pos(m_w(G,self.ge))
                    G.set_max(l) ifself.ge else G.set_min(l)
                
                if G has not true literal
                    self.mps = self.mps - m_w + self.weight[m_w(G,self.ge)]
            '''

            if m_und is None:
                G.set_max(l) if self.ge else G.set_min(l)
                if tg is None:
                    if self.choice_cons == "AMO":
                        self._mps += w_l
                    else:
                        assert False
                continue
            
            pos_m     =  G.ord_i[m_und]
            pos_l     =  G.ord_i[l]
            m_weight  =  self.weight[m_w(G, max = self.ge)]
           

            if tg == l:

                # updating the self.mps
                if (m_weight > w_l and self.ge) or (m_weight < w_l and not self.ge): 
                    self._mps = self._mps - w_l + m_weight

                # updating the max undefined if self.ge else min undefined
                if (self.ge and pos_m < pos_l) or (not self.ge and pos_m > pos_l):
                    G.set_max(l) if self.ge else G.set_min(l)

            else:
                
                if (self.ge and w_l >= m_weight and pos_l > pos_m) or (not self.ge and w_l <= m_weight and pos_l < pos_m ):
                    G.set_max(l) if self.ge else G.set_min(l)
                    if tg is None:
                        self._mps = self._mps - m_weight + w_l

    def compute_changes_str(self, changes, thread_id):
        changes_str = []
        for plit in changes:
            changes_str.append((get_name(atomNames=self.atomNames, lit = plit), plit))
        return changes_str

    def create_propagator(atomNames, sys_parameters, propagation_phase, ge, choice_cons, solver=WASP) -> "AmoSumPropagator":
        propagator = AmoSumPropagator(atomNames=atomNames, sys_parameters=sys_parameters, propagation_phase=propagation_phase, ge=ge, choice_cons=choice_cons, solver=solver)
        return propagator

    def getReasonsForCheckFailure(self):
        return None

    def checkAnswerSet(self, *answer_set):

        write = True if "write_stats_reason" in self.param else False

        if self.minimization == Minimize.NO_MINIMIZATION.value or not write:
            return wasp.coherent()

        file_to_write : str

        if self.minimization == Minimize.MINIMAL.value:
            file_to_write = settings.STATISTICS_REASON_FILE_MINIMAL
        elif self.minimization == Minimize.CARDINALITY_MINIMAL.value:
            file_to_write = settings.STATISTICS_REASON_FILE_MINIMUM
        else:
            assert False

        try:
            with open(file_to_write, 'a') as file:
                if self.count_p != 0:
                    file.write(f"{self.sum_p},{self.count_p}")
        except IOError as e:
            print(f"An error occurred: {e}")

        self.sum_p = 0
        self.count_p = 0

        return wasp.coherent()

    def updated_dl(self, lit, dl):
        self.last_decision_lit = lit if dl != self.dl else self.last_decision_lit
        self.dl = dl

    def is_in_aggregate(self, l):
        return self.aggregate[l] or self.aggregate[not_(l)]