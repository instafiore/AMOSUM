import sys
import os
sys.path.append(os.path.dirname(os.path.abspath(__file__)))
from ast import Tuple
from typing import Callable, List
from utility import *
import wasp.wasp as wasp
import re
import settings



class Propagator:

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

    # (Case one literal per group): a function from literals -> self.groups
    # (Case multiple literal per group (MLG): a function from literals -> [self.groups]
    group : GroupFunction

    # a function from self.groups -> literals U {None}
    true_group : TrueGroupFunction

    # a set of self.groups
    groups : List[Group]

    # literals derived at level 0
    facts : List[int]

    # reason for literals 
    reason : List[int] 

    # reason for true_literals (no more usefull)
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
    propagate_phase : Callable[[Group, 'Propagator', dict],List[int]]

    # propagate function to implement in propagator file
    onLiteralsUndefined: Callable[[Tuple], None]


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

    # SUPPORTED SOLVERS
    WASP = 1
    CLINGO = 2

    def __init__(self, atomsNames: dict[str: int], sys_parameters: List[str], propagation_phase: Callable[[Group, 'Propagator'],  List[int]] = None, ge: bool = True, prob_type: str = "AMO", solver = WASP) -> None:
        self.atomNames = atomsNames
        self.sys_parameters = sys_parameters
        self.facts : List[int] = []
        self.reason : List[int] = []
        self.strategy = 'default'
        self.last_decision_lit = 0
        self.minimization = Minimize.NO_MINIMIZATION
        self.sum_p = 0
        self.count_p = 0
        self.ge = ge
        self.prob_type = prob_type
        self.propagate_phase = propagation_phase
        self.solver = solver

    def getReasonsForCheckFailure(self):
        return None
    
    def getReason(self):
        return self.reason
    
    def getReasonForLiteral(self, lit):
    
        reason = self.reason + self.reason_trues[lit]
        rl = self.redundant_lits[lit] 
        removed = False
        reason_c = reason
        self.reason_trues[lit] = []

        # removing redundant lits (if any)
        if len(rl) > 0:
            removed = True
            reason = remove_elements(reason, rl)
        
        print_reason(atomNames=self.atomNames, R=reason, literal=lit)
        
        # printing/updating reduction statistics
        if removed:
            lc = len(reason_c)
            p = (1 - (len(reason) / lc)) if lc > 0 else 1
            p *= 100
            self.sum_p += p
            self.count_p += 1
            print_reduction_reason(self, reason_c, reason, lit)

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
    
        set_debug(param.get("d",""))
       
        assert param != "ERROR"
        debug(f"param {param}", force_print=True)
        
        # initializing 
        self.minimization = param.get("min_r",Minimize.NO_MINIMIZATION.value)
        self.strategy = param.get("strategy",self.strategy)
        self.N = lits[0] + 1
        self.I = SymmetricFunction(self.N)
        self.weight = WeightFunction(self.N)
        self.group = GroupFunction(self.N, default = [])
        self.aggregate = AggregateFunction(self.N, False)
        self.reason_trues = PerfectHash(self.N,[])
        self.redundant_lits = PerfectHash(self.N,[])
        self._mps = 0
        self.ID = param.get("id","0")
        # self.groups = set()
        self.groups = []
        self.assumptions = param.get("ass", False)

        #used to create the self.groups
        groups_raw : dict[int, List[int]] = {}

        self.lb = None
        bind = []
        negative_lit_regex = re.compile(r"^not\s+(?P<atom_name>[\w()]+)")
        bound_str = "lb" if self.ge else "ub" 
        bound = None
         

        # selecting the interested literals

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
                group_id = terms[2]

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
        
            # adding the self.group to the set of self.groups
            # self.groups.add(G)
            assert G not in self.groups
            self.groups.append(G)


            # defining the function self.group
            for lit in lits_group:
                self.group[lit].append(G)
        
        nGroup = Group.autoincrement 
        self.true_group = TrueGroupFunction(nGroup)

        # PREPROCESSING
        for i in range(1,len(lits)):
            l = lits[i]
            try:
                self.update_phase(l)
                self.inconsistent_at_level_0 = False
            except Exception as e:
                self.inconsistent_at_level_0 = True

        self.facts = lits[1:]
        self.param = param

        self.last_decision_lit = 1
        self.dl = 0        

        return bind 

    def simplifyAtLevelZero(self, delete_lits = False):

        # INCOHERENT
        error_string = "self.mps < self.lb !!!" if self.ge else "self.mps > self.ub !!!"
        if (self.ge and self._mps < self.lb) or (not self.ge and self._mps > self.ub) :
            debug(error_string)
            return [1]
         
        assert not self.inconsistent_at_level_0
        
        prop_from_facts = self.propagate_phase(None, self, self.atomNames)
        
        if delete_lits:
            simplifyLiterals(self.facts, self.aggregate, self.group, max = self.ge, I = self.I)

        

        if self.assumptions:
            self.assumptions = convert_assparam_to_assarray(self.assumptions)

        return  create_assumptions_lits(assumptions=self.assumptions,atomNames=self.atomNames) + prop_from_facts

    def updated_dl(self, lit, dl):
        self.last_decision_lit = lit if dl != self.dl else self.last_decision_lit
        self.dl = dl

    def onLiteralTrue(self, lit, dl):

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
        name = get_name(lit=lit, atomNames=self.atomNames)
        if next_phase:
            try:
                debug(f"[{next_phase}] Propagation phase of {name} started [{self._mps}]:")
                propagated_lits = self.propagate_phase(G, self, self.atomNames)
            except Exception as e:
                print(e, file=sys.stderr)
                raise e
        else:
            debug(f"[{next_phase}] Propagation phase of {name} not started [{self._mps}]:")


        return propagated_lits


    def update_phase(self, l: int) -> tuple[bool, Group]:
    
        self.I[l] = True
        tg = False
        G : Group

        if self.aggregate[l]:
            tg = True 
        elif self.aggregate[not_(l)]:
            pass
        else:

            return (False, None)

        mps_prev = self._mps

        groups_l = self.group[l] if tg else self.group[not_(l)]

        # Propagation is required since the second most undefined has became defined
        amo_condition : bool = False
        for G in groups_l:
            w_p : int = 0
            w_n : int = 0
            G.decrease_und()
            if tg:
                self.true_group[G] = l
                w_p = self.weight[m_w(G, max = self.ge)]
                w_n = self.weight[l]
            else:
                new, prev = G.update(self.I, max=self.ge, update=False, assuming_und = l)
                if not_(l) == prev:
                    G.set_max_min(l=new, max=self.ge)
                    if self.true_group[G] is None :
                        w_n = self.weight[new]
                        w_p = self.weight[prev]
                elif not_(l) != new:
                    # there is at least one more literal that can satify the lower bound
                    # so no propagation can take place
                    pass
                elif self.prob_type == "AMO":
                    amo_condition = True
                else:
                    pass
            
            if w_p == w_n:
                continue

            self._mps = self._mps - w_p + w_n
            assert_mps : bool
            if self.ge:
                assert_mps = self._mps >= self.lb 
            else:
                assert_mps = self._mps <= self.ub


            if not assert_mps:
                name = get_name(atomNames=self.atomNames, lit=l)
                error = f"{name} true led the mps {self._mps} to be incosistent with {self.bound}"
                debug(error)
                if self.solver != Propagator.WASP or self.prob_type != "EO":
                    raise Exception(error)

        return (mps_prev != self._mps or amo_condition,  None)
    
    def mps(self, l: int, assumed:bool, return_literals = False):
        groups_l = self.group[l]
        if len(groups_l) == 0:
            groups_l = self.group[not_(l)]
        delta_mps = 0
        for g in groups_l:
            if assumed:
                m = g.get_most_undefined(max=self.ge)
                ml_g = g.ord_l[m] if not m is None else None
                mw_g = self.weight[ml_g]
                assert self.true_group[g] is None
                delta_mps =  - mw_g + self.weight[l]
            else:
                sml_g, ml_g =  g.update(self.I, update=False, max=self.ge)
                mw_g =  self.weight[ml_g]
                if ml_g != l:
                    continue
                delta_mps = - mw_g + self.weight[sml_g]
        mps = self._mps + delta_mps 
        return mps if not return_literals else (mps, sml_g, ml_g)
        
    def mps_k(self, g: Group, l: int, assumed:bool, return_literals = False):
        if assumed:
            if l in m_undef_set_g:
                return self._mps if not return_literals else (self._mps , sml_g, m_undef_set_g)
            # returning the least undefined
            m = g.get_least_undefined(max=self.ge)
            ml_g = g.ord_l[m] if not m is None else None
            mw_g = self.weight[ml_g]
            assert self.true_group[g] is None
            mps = self._mps - mw_g + self.weight[l]
            return mps if not return_literals else (mps, l, ml_g)
        else:
            # returning 
            # 1) returning the possible new least undefined if the real one was false
            # 2) returning the undefined set
            sml_g, m_undef_set_g =  g.update(self.I, update=False, max=self.ge, return_m_undef_set_g = True)
            if not l in m_undef_set_g:
                return self._mps if not return_literals else (self._mps , sml_g, m_undef_set_g)
            mps = self._mps - self.weight[l] + self.weight[sml_g]
            return mps if not return_literals else (mps, sml_g, ml_g)

    def compute_minimal_reason(self, reason: List[int], derived: List[int]):
        '''
        Invariants (in case of cmin strategy) reason is grouped by self.group id and in each self.group the literals are sort in descending order
        '''

        if self.minimization == Minimize.NO_MINIMIZATION.value:
            return
        
        for l in derived:
            groups_l = self.group[l]
            derived_true = True
            if len(groups_l) == 0:
                groups_l = self.group[not_(l)]
                derived_true = False
            assert len(groups_l) > 0 
    
            mps = self.mps(l, assumed = not derived_true)
            s = self.lb - mps - 1 
    
            if self.minimization == Minimize.MINIMAL.value:
                self.redundant_lits[l] = maximal_subset_sum_less_than_s_with_groups(literals=reason + self.reason_trues[l], s = s, weight= self.weight, group=self.group, head_reason=l, I=self.I, max=self.ge)
            elif self.minimization == Minimize.CARDINALITY_MINIMAL.value:
                # TODO: Refactor, a lot of stuff remained outdate like group, it is still a function to a single gropu instead to a list of groups
                increment = compute_increment_literals(literals=reason + self.reason_trues[l], group=self.group, weight=self.weight)
                self.redundant_lits[l]  = maximum_subset_sum_less_than_s_with_groups(literals= reason, s = s, weight = increment, group=self.group, I=self.I)            
            else:
                assert False

    def onLiteralsUndefined(self, *lits, wasp: bool = True) -> None:
        
        for i in range(1 if wasp else 0,len(lits)):
            l = lits[i]
            
            # This has been added to handle early stop in propagation phase (clingo propagation)
            if self.I[l] is None:
                continue


            # updating interpretation
            self.I[l] = None


            # updating max self.weight for self.group(l)
            groups_l = self.group[l] 

            if len(groups_l) == 0:
                groups_l = self.group[not_(l)]
                l = not_(l)

            for G in groups_l:

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
                            self._mps += self.weight[l]
                        else:
                            assert False
                    continue
                
                pos_m     =  G.ord_i[m_und]
                pos_l     =  G.ord_i[l]
                m_weight  =  self.weight[m_w(G, max = self.ge)]
                
                if tg == l:

                    # updating the self.mps
                    if (m_weight > self.weight[l] and self.ge) or (m_weight < self.weight[l] and not self.ge): 
                        self._mps = self._mps - self.weight[l] + m_weight

                    # updating the max undefined if self.ge else min undefined
                    if (self.ge and pos_m < pos_l) or (not self.ge and pos_m > pos_l):
                        G.set_max(l) if self.ge else G.set_min(l)

                else:
                    w_l = self.weight[l]
                    if (self.ge and w_l >= m_weight and pos_l > pos_m) or (not self.ge and w_l <= m_weight and pos_l < pos_m ):
                        G.set_max(l) if self.ge else G.set_min(l)
                        if tg is None:
                            self._mps = self._mps - m_weight + w_l

    def compute_changes_str(self, changes, thread_id):
        changes_str = []
        for plit in changes:
            changes_str.append((get_name(atomNames=self.atomNames, lit = plit), plit))
        return changes_str
