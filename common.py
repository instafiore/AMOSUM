from ast import Tuple
import wasp
from typing import Callable, List
from utility import *
import re
import settings

atomNames = {}

# input parameters
sys_parameters=[]

# Aggregate id
ID : int

# N: number of atoms in the program
# n: number of atoms in the aggregate
N: int

# a function from literals -> {True, False, None}
I : SymmetricFunction

# a function from literals -> weights
# assuming that the aggregate does not contain the to literal l and !l
weight : WeightFunction

# a function from literals -> {True, False}
aggregate : AggregateFunction

# a function from literals -> groups
group : GroupFunction

# a function from groups -> literals U {None}
true_group : TrueGroupFunction

# a set of groups
groups : set[Group]

# literals derived at level 0
lits_level_0 : List[int] = []

# reason for falses literals 
reason_falses : List[int] = []

# reason for true_literals
reason_trues : PerfectHash

# redundant literals in reason of a literal l
# it is a funtion lits -> 2^(lits)
redundant_lits : PerfectHash

# assumptions as a list of atom names
assumptions : List[str]

# parameters from standard input
param : dict

# global array of ordered lit per weight (ascending order)
global_ord_lit : List[int]

# strategy with which to create the minimal reason, default is the order given in input
strategy = 'default'

# the last decision literal
last_decision_lit = 0

# type of minimization, default no minimization
minimization = Minimize.NO_MINIMIZATION

# sum of percentage of redaction of reason
sum_p = 0

# count of reducted reasons
count_p = 0

# defining the problem type, possible values: AMO, EO
PROB_TYPE : str

# defining whether the propagator is for the constraint >=  (GE) or <= (LE) 
GE : bool

# propagate function to implement in propagator file
propagate_phase : Callable[[Group],List[int]]

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

def getReasonsForCheckFailure():
    return None

def getReasonForLiteral(lit):
    global reason_falses, reason_trues, redundant_lits, sum_p, count_p

    reason = reason_falses + reason_trues[lit]
    rl = redundant_lits[lit]
    removed = False
    reason_c = reason
    if len(rl) > 0:
        removed = True
        reason = remove_elements(reason, rl)
    reason_trues[lit] = []
    
    write = True if "write_stats_reason" in param else False
    if removed and write:
        p = (1 - (len(reason) / len(reason_c))) * 100
        sum_p += p
        count_p += 1
        # redundant_lits_str = convert_array_to_string(name=f"from {len(reason_c)} to {len(reason)} removed from reason of {get_name(atomNames=atomNames, lit=lit)} lit {lit}", array=redundant_lits[lit], atomNames=atomNames)
        # print_err(redundant_lits_str)
        # redundant_lits_str = convert_array_to_string(name=f"removed from reason of {get_name(atomNames=atomNames, lit=lit)} ", array=redundant_lits[lit], atomNames=atomNames)
        # print_err(redundant_lits_str)
    redundant_lits[lit] = []

    return reason

def checkAnswerSet(*answer_set):
    global sum_p, count_p

    write = True if "write_stats_reason" in param else False

    if minimization == Minimize.NO_MINIMIZATION.value or not write:
        return wasp.coherent()

    file_to_write : str

    if minimization == Minimize.MINIMAL.value:
        file_to_write = settings.STATISTICS_REASON_FILE_MINIMAL
    elif minimization == Minimize.CARDINALITY_MINIMAL.value:
        file_to_write = settings.STATISTICS_REASON_FILE_MINIMUM
    else:
        assert False

    try:
        with open(file_to_write, 'a') as file:
            if count_p != 0:
                file.write(f"{sum_p},{count_p}\n")
    except IOError as e:
        print(f"An error occurred: {e}")

    sum_p = 0
    count_p = 0

    return wasp.coherent()


def getLiterals(*lits):
    global N,lb, ub, I, weight, aggregate, groups, mps, group, atomNames, true_group, lits_level_0, reason_trues, ID, param, assumptions, global_ord_lit, redundant_lits, strategy, minimization
    param = process_sys_parameters(sys_parameters)
    debug(f"param {param}")
    
    # initializing 
    minimization = param.get("min_r",Minimize.NO_MINIMIZATION.value)
    strategy = param.get("strategy",strategy)
    N = lits[0] + 1
    I = SymmetricFunction(N)
    weight = WeightFunction(N)
    group = GroupFunction(N)
    aggregate = AggregateFunction(N, False)
    reason_trues = PerfectHash(N,[])
    redundant_lits = PerfectHash(N,[])
    mps = 0
    ID = param.get("id",0)
    groups = set()
    # groups = []
    assumptions = param["ass"] if "ass" in param else False 
    groups = set()
    # groups = []

    #used to create the groups
    groups_raw : dict[int, List[int]] = {}

    lb = None
    bind = []
    negative_lit_regex = re.compile(r"^not\s+(?P<atom_name>[\w()]+)")
    bound_str = "lb" if GE else "ub" 
    bound = None

    # selecting the interested literals
    # debug("atomNames",atomNames)
    for a in atomNames:
        if  a.startswith('group('):
            terms = wasp.getTerms('group',a)
            # group(lit_name, weight, group_id)
            if len(terms) != 4 or terms[3] != ID:
                continue
            
            lit_str = terms[0]
            atom_name = lit_str
            match = negative_lit_regex.match(lit_str)
            if match:
                atom_name = match.group('atom_name')
                lit =  atomNames[atom_name] * -1
            else:
                lit =  atomNames[atom_name]
        
            # updating the weight
            weight[lit] = int(terms[1])

            # updating the group id
            group_id = int(terms[2])

            G = groups_raw.get(group_id,[])
            G.append(lit)
            groups_raw[group_id] = G

            # adding to the aggregate
            aggregate[lit] = True
            

            
            bind.append(lit)
            bind.append(-lit)
        
        elif a.startswith(f"{bound_str}("):
            terms = wasp.getTerms(f'{bound_str}',a)
            if (len(terms) != 2 or terms[1] != ID) and len(terms) != 1:
                continue
            if not bound is None:
                assert False     
            if GE:
                lb = int(terms[0])
            else:
                ub = int(terms[0])
            bound = lb if GE else ub 

    
    assert not bound is None


    # creating groups
    for group_id in groups_raw:
        
        lits_group = groups_raw[group_id]

        # ordering by weight
        lits_ord = [(lit, weight[lit]) for lit in lits_group]
        lits_ord = sorted(lits_ord, key = lambda x: x[1])
   
        ord_l = [None] * len(lits_ord)

        # it cannot become a PerfectHash since the space required would be O(n^2) (n number of literals)
        ord_i : dict[int, int] = {}

        for i in range(len(lits_ord)):
            l = lits_ord[i][0]
            ord_l[i] = l
            ord_i[l] = i
        
        # creating the group
        G = Group(ord_l,ord_i,group_id)
        
        # updatind the max possible sum
        mps = mps + weight[m_w(G, max = GE)]
    
        # adding the group to the set of groups
        groups.add(G)
        # groups.append(G)

        # defining the function group
        for lit in lits_group:
            group[lit] = G
    
    nGroup = Group.autoincrement 
    true_group = TrueGroupFunction(nGroup)

    # PREPROCESSING
    for i in range(1,len(lits)):
        l = lits[i]
        update_phase(l)

    lits_level_0 = lits


    return bind 

def simplifyAtLevelZero():
    global N,lb, I, weight, aggregate, groups, group, reason, assumptions, mps

    # INCOHERENT
    error_string = "MPS < LB !!!" if GE else "MPS > UB !!!"
    if (GE and mps < lb) or (not GE and mps > ub) :
        debug(error_string)
        return [1]
    
    res = propagate_phase(None)
    
    simplyLiterals(lits_level_0, aggregate, group)

    if assumptions:
        assumptions = convert_assparam_to_assarray(assumptions)

    return res + create_assumptions_lits(assumptions=assumptions,atomNames=atomNames)

def onLiteralTrue(lit, dl):
    global last_decision_lit

    last_decision_lit = lit
    debug(f"True {get_name(lit=lit, atomNames=atomNames)} id {lit} DL {dl}")
    (next_phase, G) = update_phase(lit)

    propagated_lits = []
    if next_phase:
        propagated_lits = propagate_phase(G)

    return propagated_lits

def update_phase(l: int) -> (bool, Group):
    global N, lb, ub, I, weight, aggregate, groups, mps, group, true_group

    w_p : int = 0
    w_n : int = 0
    I[l] = True
    tg = False
    G : Group

    if aggregate[l]:
        G = group[l]
        G.decrease_und()
        true_group[G] = l
        w_p = weight[m_w(G, max = GE)]
        w_n = weight[l]
        tg = True
        debug(f"w_p: {w_p} - w_n {w_n}")
        
    elif aggregate[not_(l)]:
        G = group[not_(l)]
        G.decrease_und()

        if not_(l) == m_w(G, max = GE):
            new, prev = G.update(I, max=GE)
            if true_group[G] is None :
                w_n = weight[new]
                w_p = weight[prev]
    else:
        return (False, None)

    mps = mps - w_p + w_n
    amocondition = ( G.count_undef == 1 and not tg ) and PROB_TYPE == "AMO"
    return (w_p != w_n or amocondition,  G if not amocondition else None)

def compute_minimal_reason(reason: List[int]):
    global N,lb, I, weight, aggregate, groups, mps, group, true_group, reason_falses, reason_trues, redundant_lits

    '''
    Invariants (in case of cmin strategy) reason is grouped by group id and in each group the literals are sort in descending order
    '''

    if minimization == Minimize.NO_MINIMIZATION.value:
        return

    '''
    IMPORTANT: It is not possible to eliminate the decision literal at the highest level, since it is for sure in the reason.
    Otherwise can happen the, let !li be a decision literal of the last level and !li -> !lj. If there exists
    a group G = {lj, lk, ... } where lj and all the others literals are false except for lk
    maybe lk is derived, since lj is false. Can happen that lk can be derived even if li is true, but lj is false because of li.
    So li is mandatory in the reason.
    '''
    
    for l in reason:
        reason_l_to_minimize = []
        g = group[l]
        assert not g is None
        s = lb - (mps - weight[l]) - 1 
        for lr in reason:
            glr = group[lr] 
            # it means that the literal is true, since for sure has been flipped (it has not group otherwise)
            # so cannot be the same group of l, since l is also true 
            if not glr is None and g.id == glr.id or lr == -last_decision_lit:
                continue
            reason_l_to_minimize.append(lr)

        if minimization == Minimize.MINIMAL.value:
            redundant_lits[l] = maximal_subset_sum_less_than_s_with_groups(literals=reason_l_to_minimize, s = s, weight= weight, group=group)
        elif minimization == Minimize.CARDINALITY_MINIMAL.value:
            increment = compute_increment_literals(literals=reason_l_to_minimize, group=group, weight=weight)
            redundant_lits[l]  = maximum_subset_sum_less_than_s_with_groups(literals= reason_l_to_minimize, s = s, weight = increment, group=group, I=I)            
        else:
            assert False

def onLiteralsUndefined(*lits) -> None:
    global N, lb, ub, I, weight, aggregate, groups,  mps, group, reason, true_group

    for i in range(1,len(lits)):
        l = lits[i]
        
        debug(f"Undef {get_name(lit=l, atomNames=atomNames)} id {l}")

        # updating interpretation
        I[l] = None


        # updating max weight for group(l)
        G : Group = group[l] 

        if G is None:
            G = group[not_(l)]
            l = not_(l)

        assert not G is None

        # increasing the number of undefined for G
        G.increase_und()

        tg = true_group[G]

        # the true literal of G became undefined
        if tg == l:
            true_group[G] = None

        m_und = m_w(G, max = GE)

        '''
        if G has all literals defined
            G.set_max(l) if GE else G.set_min(l)
            return

        if l was the true literal of the group 
   
            # updating the mps
            if weight[m_w(G, GE)] > if GE else < weight[l] -> mps = mps - weight[l] + weight[m_w(G, GE)]
            
            # updating the max undefined
            if pos(m_w(G, GE)) > if GE else < pos(l) -> G.set_max(l) if GE else G.set_min(l)

        else
        
            m_weight = weight[m_w(G, GE)]
            if weight[l] >= if GE else <= m_weight and pos(l) > if GE else < pos(m_w(G, GE))
                G.set_max(l) if GE else G.set_min(l)
            
            if G has not true literal
                mps = mps - m_w + weight[m_w(G, GE)]
        '''

        if m_und is None:
            G.set_max(l) if GE else G.set_min(l)
            if tg is None:
                if PROB_TYPE == "AMO":
                    mps += weight[l]
                else:
                    assert False
            continue
        
        pos_m     =  G.ord_i[m_und]
        pos_l     =  G.ord_i[l]
        m_weight  =  weight[m_w(G, max = GE)]
        
        mps_p = mps 
        if tg == l:

            # updating the mps
            if (m_weight > weight[l] and GE) or (m_weight < weight[l] and not GE): 
                mps = mps - weight[l] + m_weight

            # updating the max undefined if GE else min undefined
            if (GE and pos_m < pos_l) or (not GE and pos_m > pos_l):
                G.set_max(l) if GE else G.set_min(l)

        else:
            w_l = weight[l]
            if (GE and w_l >= m_weight and pos_l > pos_m) or (not GE and w_l <= m_weight and pos_l < pos_m ):
                G.set_max(l) if GE else G.set_min(l)
                if tg is None:
                    mps = mps - m_weight + w_l
