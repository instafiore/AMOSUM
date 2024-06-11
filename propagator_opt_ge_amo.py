#!/usr/bin/python3
import json
import utility
import wasp
from typing import List
from utility import PerfectHash, debug, Group, max_w, Interpretation, WeightFunction,\
    GroupFunction, not_, get_name, print_I, convert_array_to_string, print_perfect_hash, print_weights, print_groups, FOCUSED_GROUP, convert_assparam_to_assarray, create_assumptions_lits, \
    AggregateFunction, TrueGroupFunction, remove_elements, simplyLiterals
import re

'''
    !a -> b ^ c ^ d
    <=>
    a v !(!b v !c v !d)
    <=>
    (a v b) ^ (a v c) ^ (a v d)  
'''

atomNames = {}

'''
Propagator for ' >= LB  ' constraint and At Most One constraint
'''

sys_parameters=[]
# Aggregate id
ID : int

# N: number of atoms in the program
# n: number of atoms in the aggregate
N: int

# a function from literals -> {True, False, None}
I : Interpretation

# a function from literals -> weights
weight : WeightFunction

# a function from literals -> {True, False}
aggregate : AggregateFunction

# a function from literals -> groups
group : GroupFunction

# a function from groups -> literals U {None}
true_group : TrueGroupFunction

# a set of groups
groups : set[Group]

# lower bound
lb : int

# max possible sum
mps : int

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


def getReasonForLiteral(lit):
    global reason_falses, reason_trues, redundant_lits
    reason = reason_falses + reason_trues[lit]
    rl = redundant_lits[lit]
    if len(rl) > 0:
        reason = remove_elements(reason, rl)
    reason_trues[lit] = []
    redundant_lits[lit] = []
    return reason

def process_sys_parameters():
    global param, sys_parameters

    param = {}
    regex = r"^-(.+)" 
    
    debug(sys_parameters)
    i = 1

    while i < len(sys_parameters):
        
        # creating the key
        key = sys_parameters[i] 
        res_regex = re.match(regex, key)
        if res_regex is None:
            raise Exception("Every key has to start with a dash! Ex: -problem knapsack")
        key = res_regex.group(1)

        if i + 1 >= len(sys_parameters) :
            param[key] = True
            break
        
        value = sys_parameters[i+1] 
        res_regex = re.match(regex, value)
        if res_regex is None:
            i += 2
            param[key] = value
            
        else:
            i += 1
            param[key] = True

    

def getLiterals(*lits):
    global N,lb, I, weight, aggregate, groups, mps, group, atomNames, true_group, lits_level_0, reason_trues, ID, param, assumptions, global_ord_lit, redundant_lits, strategy

    process_sys_parameters()

    debug("param", param)

    lb = None
    bind = []
    negative_lit_regex = re.compile(r"^not\s+(?P<atom_name>[\w()]+)")

    strategy = param.get("strategy",strategy)

    # initializing 
    N = lits[0] + 1
    I = Interpretation(N)
    weight = WeightFunction(N)
    group = GroupFunction(N)
    aggregate = AggregateFunction(N, False)
    reason_trues = PerfectHash(N,[])
    redundant_lits = PerfectHash(N,[])
    global_ord_lit = []
    mps = 0
    ID = param["id"]

    assumptions = param["ass"] if "ass" in param else False

    #used to create the groups
    groups_raw : dict[int, List[int]] = {}
    groups = set()

    # selecting the interested literals
    for a in atomNames:
        debug("Atom",a, end=" ")
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
            
            # adding the literal to the global array of literals
            global_ord_lit.append(lit)
            
            bind.append(lit)
            bind.append(-lit)
        
        elif a.startswith("lb("):
            terms = wasp.getTerms('lb',a)
            if len(terms) != 2 or terms[1] != ID:
                continue
            if not lb is None:
                assert False     
            lb = int(terms[0])
    debug("",end="\n")
    assert not lb is None

    # ordering literals in the global array
    global_ord_lit = [(lit, weight[lit]) for lit in global_ord_lit]
    global_ord_lit = sorted(global_ord_lit, key = lambda x: x[1])
    global_ord_lit = [lit for lit, w in global_ord_lit]

    # creating groups
    for group_id in groups_raw:
        
        lits_group = groups_raw[group_id]

        # ordering by weight
        lits_ord = [(lit, weight[lit]) for lit in lits_group]
        lits_ord = sorted(lits_ord, key = lambda x: x[1])
   
        ord_l = [None] * len(lits_ord)

        # it cannot become a PerfectHash since the space required would be O(n^2) (n number of nodes)
        ord_i : dict[int, int] = {}

        for i in range(len(lits_ord)):
            l = lits_ord[i][0]
            ord_l[i] = l
            ord_i[l] = i
        
        # creating the group
        G = Group(ord_l,ord_i,group_id)
        
        # updatind the max possible sum
        mps = mps + weight[max_w(G)]
    
        # adding the group to the set of groups
        groups.add(G)

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
    global N,lb, I, weight, aggregate, groups, group, reason, assumptions


    # INCOHERENT
    if mps < lb:
        debug("MPS < LB !!!")
        return [1]
    
    res = propagate_phase(None)
    
    simplyLiterals(lits_level_0, aggregate, group)


    if assumptions:
        assumptions = convert_assparam_to_assarray(assumptions)

    return res + create_assumptions_lits(assumptions=assumptions,atomNames=atomNames)

def onLiteralTrue(lit, dl):
    global N,lb, I, weight, aggregate, groups, mps, group, true_group

    debug("True",get_name(lit=lit, atomNames=atomNames), "DL", dl)
    (next_phase, G) = update_phase(lit)

    propagated_lits = []
    if next_phase:
        propagated_lits = propagate_phase(G)

    return propagated_lits    

def update_phase(l: int) -> (bool, Group):
    global N,lb, I, weight, aggregate, groups, mps, group, true_group
    I[l] = True
    G : Group
    if aggregate[l]:
        G = group[l]
        G.decrease_und()
        true_group[G] = l
        w_max = weight[max_w(G)]
        mps = mps - w_max + weight[l]
        if w_max == weight[l]:
            return (False, G)
        
    elif aggregate[not_(l)]:
        G = group[not_(l)]
        G.decrease_und()
        if not_(l) == max_w(G):
            new_max, prev_max = G.update_max(I)
            if true_group[G] is None :
                w_n : int  = 0
                w_n = weight[new_max]
                w_p = weight[prev_max]
                mps = mps - w_p + w_n  
            else:
                return (True, G)   
        elif G.count_undef == 1:
            return (True, G)
        else:
            return (False, G)      
    else:
        return (False, None)

    # the mps has changed
    return (True, G)


def propagate_phase(G: Group):
    
    global N,lb, I, weight, aggregate, groups, mps, group, true_group, reason_falses, reason_trues

    # set of derived literals
    S : List[int] = []
    
    # reason
    R : List[int] = []

    trues = []

    for g in groups:

        ml_g =  max_w(g)
        mw_g = weight[ml_g]

        count_infered_falses = 0 
        false_lits_g = []
        # infer falsity
        for l in g.ord_l:
            if I[l] is None:
                if mps - mw_g + weight[l] < lb:
                    S.append(not_(l))
                    count_infered_falses += 1
                else:
                    break
            elif I[l] is False:
                false_lits_g.append(l)

        if g.count_undef - count_infered_falses == 1 and true_group[g] is None:
            # the last remained literal 
            l = max_w(g)
            if mps - weight[l] < lb:
                S.append(l)
                reason_trues[l] = false_lits_g
                trues.append(l)
    if len(S) != 0:
        for g in groups:
            if g.count_undef == 0 and true_group[g] is None:
                R.extend(g.ord_l)
            elif true_group[g] is None:
                mw_g = weight[max_w(g)]
                for i in range(len(g.ord_l) - 1, -1, -1):
                    l = g.ord_l[i]
                    if weight[l] <= mw_g:
                        break
                    R.append(l)
            else:
                R.append(not_(true_group[g]))

        # updating the reason
        reason_falses = R

    S_str = convert_array_to_string(name="Derived", array=S, atomNames=atomNames)
    debug(S_str)
    R_str = convert_array_to_string(name="Reason", array=R, atomNames=atomNames)
    debug(R_str)

    print_I(I=I, atomNames=atomNames, aggregate=aggregate)
    compute_minimal_reason(reason=R, trues=trues)
     
    return S

def get_increment_name(increment):
    increment_name = {}
    for i in increment:
        increment_name[f"{get_name(atomNames=atomNames,lit=i)}"] = increment[i]
    return increment_name

def compute_increment_literals(literals):
    global N,lb, I, weight, aggregate, groups,  mps, group, true_group, global_ord_lit, redundant_lits, reason_trues
    increment = {}
    for l in literals:
        g : Group
        g = group[l] 
        if g is None:
            g = group[not_(l)]
        assert not g is None 
        mw_g = max_w(g)
        w_mw_g = weight[mw_g]
        w = weight[l]
        if w is None:
            w =  weight[not_(l)]
             # it means that the literal has been flipped in the reason
            l = not_(l)
        assert not w  is None
        i : int
        if I[l] == True:  
            w_mw_g = weight[g.ord_l[-1]]
            # if is true: if it was false in the worst case the maxiumum literal of the group could be true
            i = w_mw_g - w
        elif I[l] == False:
            # if l false: if it was true it would be the true_group[g] 
            i = w - w_mw_g
        increment[l] = i

    return increment

def increment_f(l: int, current_subset_maximal):
    g = group[l] 
    w = weight[l]
    if I[l] == True:  
        w_mw_g = weight[g.ord_l[-1]]
        i = w_mw_g - w



def compute_minimal_reason(reason: List[int], trues: List[int]):
    global N,lb, I, weight, aggregate, groups,  mps, group, true_group, global_ord_lit, redundant_lits, reason_trues, strategy_minimal

    # computing increment for each literal 
    increment = compute_increment_literals(literals=reason)
    debug(f"increment {get_increment_name(increment=increment)}")
    
    for l in trues:
        redundant_lits_l = []
        g = group[l]
        assert not g is None
        increment_l = 0
        s = lb - (mps - weight[l]) - 1 
        for lr in reason:
            lr_copy = lr
            glr = group[lr] 
            if g is None:
                g = group[not_(lr)]
                lr = not_(lr)
            if g.id == glr.id:
                continue
            # if increment_l + increment[lr] <= s:
            #     increment_l += increment[lr]
            #     redundant_lits_l.append(lr_copy)
        redundant_lits[l] = redundant_lits_l
        if len(redundant_lits_l) != 0: 
            redundant_lits_str = convert_array_to_string(name=f"redundant lits for {get_name(atomNames=atomNames,lit=l)}", array=redundant_lits_l, atomNames=atomNames)
            debug(redundant_lits_str)

def maximal_subset_sum_with_groups(set: List[int], s: int, w: dict):
    global N,lb, I, weight, aggregate, groups,  mps, group, true_group, global_ord_lit, redundant_lits, reason_trues


def compute_minimum_reason(reason: List[int], trues: List[int]):
    global N,lb, I, weight, aggregate, groups,  mps, group, true_group, global_ord_lit, redundant_lits, reason_trues

    for l in trues:
        g = group[l]
        literals_l = []
        assert not g is None
        s = lb - (mps - weight[l]) - 1 
        for lr in reason:
            lr_copy = lr
            glr = group[lr] 
            if g is None:
                g = group[not_(lr)]
                lr = not_(lr)
            if g.id != glr.id:
                literals_l.append(lr)
            
        redundant_lits_l = maximum_subset_sum_less_than_s_with_groups(s=s, literals= literals_l, weight = increment)
        redundant_lits[l] = redundant_lits_l
        if len(redundant_lits_l) != 0: 
            redundant_lits_str = convert_array_to_string(name=f"redundant lits for {get_name(atomNames=atomNames,lit=l)}", array=redundant_lits_l, atomNames=atomNames)
            debug(redundant_lits_str)

    
def minimize_reason_old(reason: List[int], trues: List[int]):
    global N,lb, I, weight, aggregate, groups,  mps, group, true_group, global_ord_lit, redundant_lits, reason_trues

    for li in trues:
        redundant_lits_li = []
        gi = group[li]
        wli = weight[li]
        for lj in global_ord_lit:
            gj = group[lj]
            if gi.id == gj.id:
                continue
            if not I[lj] is None:
                if I[lj] == True and not not_(lj) in reason: 
                    continue
                elif I[lj] == False and not lj in reason:
                    continue
                tg = true_group[gj]
                mw_g = max_w(gj)


                if I[lj] == True:  
                    # tg == lj
                    if mps - weight[lj] + weight[mw_g] - wli < lb:
                        redundant_lits_li.append(not_(lj))
                    else:
                        break

                elif I[lj] == False and tg is None and weight[lj] > weight[mw_g]:
                    # debug("mps",mps,"weight[mw_g]",weight[mw_g],"weight[lj]",weight[lj],"gj.id", gj.id)
                    if mps - weight[mw_g] + weight[lj] - wli < lb:
                        redundant_lits_li.append(lj)
                    else:
                        break
                    
                    '''
                    mps - wli >= lb
                    
                    mps' - wli
                    '''

        if len(redundant_lits_li) != 0:
            reason_s = convert_array_to_string(name = "reason", array=reason + reason_trues[li] + [li],atomNames=atomNames)
            debug(convert_array_to_string(f"redundant literals for reason: {reason_s}", array=redundant_lits_li, atomNames=atomNames))
        redundant_lits[li] = redundant_lits_li



def onLiteralsUndefined(*lits):
    global N,lb, I, weight, aggregate, groups,  mps, group, reason, true_group
    
    for i in range(1,len(lits)):

        l = lits[i]

        # updating interpretation
        I[l] = None


        # updating max weight for group(l)
        G : Group = group[l] 

        if G is None:
            G = group[not_(l)]
            l = not_(l)

        # increasing the number of undefined for G
        G.increase_und()

        tg = true_group[G]

        # the true literal of G became undefined
        if tg == l:
            true_group[G] = None

        max_und = max_w(G)
        debug("Undef",get_name(atomNames=atomNames, lit=l), "tg",\
               get_name(atomNames=atomNames, lit=tg), "max_und", \
                get_name(atomNames=atomNames, lit=max_und))

        '''
        if G has all literals defined
            G.set_max(l)
            return

        if l was the true literal of the group 
   
            # updating the mps
            if weight[mw(G)] > weight[l] -> mps = mps - weight[l] + weight[mw(G)]
            
            # updating the max undefined
            if pos(mw(G)) < pos(l)  -> G.set_max(l)

        else
        
            max_w = weight[mw(G)]
            if weight[l] >= max_w and pos(l) > pos(mw(G))
                G.set_max(l)
            
            if G has not true literal
                mps = mps - max_w + weight[mw(G)]
        '''

        if max_und is None:
            G.set_max(l)
            if true_group[G] is None:
                mps += weight[l]
            continue
        
        pos_max = G.ord_i[max_und]
        pos_l   = G.ord_i[l]
        maxw = weight[max_w(G)]
        
        mps_p = mps 
        if tg == l:

            # updating the mps
            if maxw > weight[l]: 
                mps = mps - weight[l] + maxw

            # updating the max undefined
            if pos_max < pos_l:
                G.set_max(l)

        else:
            w_l = weight[l]
            if w_l >= maxw and pos_l > pos_max:
                G.set_max(l)
                if tg is None:
                    mps = mps - maxw + w_l


        

        


            

        

