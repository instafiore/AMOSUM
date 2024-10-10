import sys
import os
sys.path.append(os.path.dirname(os.path.abspath(__file__)))
from ast import Tuple
from typing import Callable, List
from utility import *
import wasp_dir.wasp as wasp
import re
import settings



class PropagatorWasp:

    atomNames : dict[str: int]

    # input parameters
    sys_parameters : List[str]

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

    # a set of self.groups
    groups : set[Group]

    # literals derived at level 0
    facts : List[int]

    # reason for falses literals 
    reason_falses : List[int] 

    # reason for true_literals
    reason_trues : PerfectHash

    # redundant literals in reason of a literal l
    # it is a funtion lits -> 2^(lits)
    redundant_lits : PerfectHash

    # self.assumptions as a list of atom names
    assumptions : List[str]

    # parameters from standard input
    param : dict

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

    # defining the problem type, possible values: AMO, EO
    prob_type : str

    # defining whether the propagator is for the constraint >=  (ge) or <= (le) 
    ge : bool

    # propagate function to implement in propagator file
    propagate_phase : Callable[[Group, 'PropagatorWasp', dict],List[int]]

    # propagate function to implement in propagator file
    onLiteralsUndefined: Callable[[Tuple], None]


    # >= (ge) constraint
    # ----------------------------
    # lower bound
    lb : int

    # max possible sum
    mps : int 
    # ----------------------------

    # <= (ge) constraint
    # ----------------------------
    # upper bound
    ub : int

    # min possible sum
    mps : int 
    # ----------------------------

    def __init__(self, atomsNames: dict[str: int], sys_parameters: List[str], propagation_phase: Callable[[Group, 'PropagatorWasp'], List[int]] = None, ge: bool = True, prob_type: str = "AMO") -> None:
        self.atomNames = atomsNames
        self.sys_parameters = sys_parameters
        self.facts : List[int] = []
        self.reason_falses : List[int] = []
        self.strategy = 'default'
        self.last_decision_lit = 0
        self.minimization = Minimize.NO_MINIMIZATION
        self.sum_p = 0
        self.count_p = 0
        self.ge = ge
        self.prob_type = prob_type
        self.propagate_phase = propagation_phase

    def getReasonsForCheckFailure(self):
        return None

    def getReasonForLiteral(self, lit):
    
        reason = self.reason_falses + self.reason_trues[lit]
        rl = self.redundant_lits[lit]
        removed = False
        reason_c = reason
        if len(rl) > 0:
            removed = True
            reason = remove_elements(reason, rl)
        self.reason_trues[lit] = []
        
        write = True if "write_stats_reason" in self.param else False

        if removed and write:
            p = (1 - (len(reason) / len(reason_c))) * 100
            self.sum_p += p
            self.count_p += 1
            # self.redundant_lits_str = convert_array_to_string(name=f"from {len(reason_c)} to {len(reason)} removed from reason of {get_name(self.atomNames=self.atomNames, lit=lit)} lit {lit}", array=self.redundant_lits[lit], self.atomNames=self.atomNames)
            # print_err(self.redundant_lits_str)
            # self.redundant_lits_str = convert_array_to_string(name=f"removed from reason of {get_name(self.atomNames=self.atomNames, lit=lit)} ", array=self.redundant_lits[lit], self.atomNames=self.atomNames)
            # print_err(self.redundant_lits_str)
        self.redundant_lits[lit] = []

        return reason

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
                    file.write(f"{self.sum_p},{self.count_p}\self.N")
        except IOError as e:
            print(f"An error occurred: {e}")

        self.sum_p = 0
        self.count_p = 0

        return wasp.coherent()


    def getLiterals(self, *lits):
        param = process_sys_parameters(self.sys_parameters) if type(self.sys_parameters) == list else self.sys_parameters if type(self.sys_parameters) == dict else "ERROR"
        assert param != "ERROR"
        debug(f"param {param}")
        
        # initializing 
        self.minimization = param.get("min_r",Minimize.NO_MINIMIZATION.value)
        self.strategy = param.get("strategy",self.strategy)
        self.N = lits[0] + 1
        self.I = SymmetricFunction(self.N)
        self.weight = WeightFunction(self.N)
        self.group = GroupFunction(self.N)
        self.aggregate = AggregateFunction(self.N, False)
        self.reason_trues = PerfectHash(self.N,[])
        self.redundant_lits = PerfectHash(self.N,[])
        self.mps = 0
        self.ID = param.get("id","0")
        self.groups = set()
        # self.groups = []
        self.assumptions = param.get("ass", False)

        #used to create the self.groups
        groups_raw : dict[int, List[int]] = {}

        self.lb = None
        bind = []
        negative_lit_regex = re.compile(r"^not\s+(?P<atom_name>[\w()]+)")
        bound_str = "lb" if self.ge else "ub" 
        bound = None

        # selecting the interested literals
        # debug(f"lits {lits}")
        # debug("self.atomNames",self.atomNames)
        for a in self.atomNames:
            if  a.startswith('group('):
                terms = wasp.getTerms('group',a)
                # Syntax: group( lit_name, weight, group_id, aggregate_id)
                if len(terms) != 4 or terms[3] != self.ID:
                    continue
                
                lit_str = terms[0]
                atom_name = lit_str
                match = negative_lit_regex.match(lit_str)
                if match:
                    atom_name = match.group('atom_name')
                    lit =  self.atomNames[atom_name] * -1
                else:
                    lit =  self.atomNames[atom_name]
            
                # updating the self.weight
                self.weight[lit] = int(terms[1])

                # updating the self.group id
                group_id = int(terms[2])

                G = groups_raw.get(group_id,[])
                G.append(lit)
                groups_raw[group_id] = G

                # adding to the self.aggregate
                self.aggregate[lit] = True
                
                bind.append(lit)
                bind.append(-lit)
            
            elif a.startswith(f"{bound_str}("):
                terms = wasp.getTerms(f'{bound_str}',a)
                if (len(terms) != 2 or terms[1] != self.ID) and len(terms) != 1:
                    continue
                if not bound is None:
                    assert False     
                if self.ge:
                    self.lb = int(terms[0])
                else:
                    self.ub = int(terms[0])
                bound = self.lb if self.ge else self.ub

        assert not bound is None


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
            self.mps = self.mps + self.weight[m_w(G, max = self.ge)]
        
            # adding the self.group to the set of self.groups
            self.groups.add(G)
            # self.groups.append(G)

            # defining the function self.group
            for lit in lits_group:
                self.group[lit] = G
        
        nGroup = Group.autoincrement 
        self.true_group = TrueGroupFunction(nGroup)

        # PREPROCESSING
        for i in range(1,len(lits)):
            l = lits[i]
            self.update_phase(l)

        self.facts = lits[1:]
        self.param = param
        return bind 

    def simplifyAtLevelZero(self):

        # INCOHERENT
        error_string = "self.mps < self.lb !!!" if self.ge else "self.mps > self.ub !!!"
        if (self.ge and self.mps < self.lb) or (not self.ge and self.mps > self.ub) :
            debug(error_string)
            return [1]
        
        prop_from_facts = self.propagate_phase(None, self, self.atomNames)
        
        # TODO: try to add prop_from_facts to facts at level 0
        simplyLiterals(self.facts, self.aggregate, self.group)

        if self.assumptions:
            self.assumptions = convert_assparam_to_assarray(self.assumptions)

        return prop_from_facts + create_assumptions_lits(assumptions=self.assumptions,atomNames=self.atomNames)

    def onLiteralTrue(self, lit, dl):
        global last_decision_lit

        last_decision_lit = lit
        debug(f"True {get_name(lit=lit, atomNames=self.atomNames)} id {lit} DL {dl}")
        (next_phase, G) = self.update_phase(lit)

        propagated_lits = []
        if next_phase:
            propagated_lits = self.propagate_phase(G, self, self.atomNames)

        return propagated_lits

    def update_phase(self, l: int) -> tuple[bool, Group]:
    
        w_p : int = 0
        w_n : int = 0
        self.I[l] = True
        tg = False
        G : Group

        if self.aggregate[l]:
            G = self.group[l]
            G.decrease_und()
            self.true_group[G] = l
            w_p = self.weight[m_w(G, max =self.ge)]
            w_n = self.weight[l]
            tg = True
            
        elif self.aggregate[not_(l)]:
            G = self.group[not_(l)]
            G.decrease_und()

            if not_(l) == m_w(G, max =self.ge):
                new, prev = G.update(self.I, max=self.ge)
                if self.true_group[G] is None :
                    w_n = self.weight[new]
                    w_p = self.weight[prev]
        else:
            return (False, None)

        self.mps = self.mps - w_p + w_n
        amocondition = ( G.count_undef == 1 and not tg ) and self.prob_type == "AMO"

        return (w_p != w_n or amocondition,  G if not amocondition else None)

    def compute_minimal_reason(self, reason: List[int]):
        '''
        Invariants (in case of cmin strategy) reason is grouped by self.group id and in each self.group the literals are sort in descending order
        '''

        if self.minimization == Minimize.NO_MINIMIZATION.value:
            return

        '''
        IMPORTANT: It is not possible to eliminate the decision literal at the highest level, since it is for sure in the reason.
        Otherwise can happen the, let !li be a decision literal of the last level and !li -> !lj. If there exists
        a self.group G = {lj, lk, ... } where lj and all the others literals are false except for lk
        maybe lk is derived, since lj is false. Can happen that lk can be derived even if li is true, but lj is false because of li.
        So li is mandatory in the reason.
        '''
        
        for l in reason:
            reason_l_to_minimize = []
            g = self.group[l]
            assert not g is None
            s = self.lb - (self.mps - self.weight[l]) - 1 
            for lr in reason:
                glr = self.group[lr] 
                # it means that the literal is true, since for sure has been flipped (it has not self.group otherwise)
                # so cannot be the same self.group of l, since l is also true 
                if not glr is None and g.id == glr.id or lr == -last_decision_lit:
                    continue
                reason_l_to_minimize.append(lr)

            if self.minimization == Minimize.MINIMAL.value:
                self.redundant_lits[l] = maximal_subset_sum_less_than_s_with_groups(literals=reason_l_to_minimize, s = s, weight= self.weight, group=self.group)
            elif self.minimization == Minimize.CARDINALITY_MINIMAL.value:
                increment = compute_increment_literals(literals=reason_l_to_minimize, group=self.group, weight=self.weight)
                self.redundant_lits[l]  = maximum_subset_sum_less_than_s_with_groups(literals= reason_l_to_minimize, s = s, weight = increment, group=self.group, I=self.I)            
            else:
                assert False

    def onLiteralsUndefined(self, *lits, wasp: bool = True) -> None:
        
        for i in range(1 if wasp else 0,len(lits)):
            l = lits[i]
            
            debug(f"Undef {get_name(lit=l, atomNames = self.atomNames)} id {l}")

            # updating interpretation
            self.I[l] = None


            # updating max self.weight for self.group(l)
            G : Group = self.group[l] 

            if G is None:
                G = self.group[not_(l)]
                l = not_(l)

            if G is None:
                print(f"literal sinner of G none is: {get_name(atomNames=self.atomNames, lit=l)} plit {l}")
            assert not G is None

            # increasing the number of undefined literals for G
            G.increase_und()

            tg = self.true_group[G]

            # the true literal of G becomes undefined
            if tg == l:
                self.true_group[G] = None

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
                    if self.prob_type == "AMO":
                        self.mps += self.weight[l]
                    else:
                        assert False
                continue
            
            pos_m     =  G.ord_i[m_und]
            pos_l     =  G.ord_i[l]
            m_weight  =  self.weight[m_w(G, max = self.ge)]
            
            if tg == l:

                # updating the self.mps
                if (m_weight > self.weight[l] and self.ge) or (m_weight < self.weight[l] and not self.ge): 
                    self.mps = self.mps - self.weight[l] + m_weight

                # updating the max undefined if self.ge else min undefined
                if (self.ge and pos_m < pos_l) or (not self.ge and pos_m > pos_l):
                    G.set_max(l) if self.ge else G.set_min(l)

            else:
                w_l = self.weight[l]
                if (self.ge and w_l >= m_weight and pos_l > pos_m) or (not self.ge and w_l <= m_weight and pos_l < pos_m ):
                    G.set_max(l) if self.ge else G.set_min(l)
                    if tg is None:
                        self.mps = self.mps - m_weight + w_l
