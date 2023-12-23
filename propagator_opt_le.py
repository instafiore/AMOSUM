#!/usr/bin/python3
import sys
import os
sys.path.append(os.environ.get('WASP_HOME'))
import wasp
from typing import List
from utility import PerfectHash, debug, Group, mw, Interpretation, WeightFunction,\
    GroupFunction, not_, get_name, print_I, print_weights, print_groups, FOCUSED_GROUP, \
    AggregateFunction, TrueGroupFunction, simplyLiterals
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
Propagator for ' <= 1 ' constraint 
'''

sys_parameters=[]
# Aggregate id
# ID

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

def getReasonForLiteral(lit):
    global reason_falses, reason_trues
    reason = reason_falses + reason_trues[lit]
    reason_trues[lit] = []
    return reason

def getLiterals(*lits):
    global N,lb, I, weight, aggregate, groups, mps, group, atomNames, true_group, lits_level_0, reason_trues

    lb = None
    bind = []
    negative_lit_regex = re.compile(r"^not\s+(?P<atom_name>[\w()]+)")

    # initializing 
    N = lits[0] + 1
    I = Interpretation(N)
    weight = WeightFunction(N)
    group = GroupFunction(N)
    aggregate = AggregateFunction(N, False)
    reason_trues = PerfectHash(N,[])
    mps = 0
    ID = sys_parameters[1]

    #used to create the groups
    groups_raw : dict[int, List[int]] = {}
    groups = set()

    # debug("atomNames", atomNames)
    
    # selecting the interested literals
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

            # debug()
            # debug("lit_name", get_name(atomNames, lit))
            # debug("weight[lit]",  weight[lit])
            # debug("group_id", group_id)
            # debug()
        
        elif a.startswith("lb("):
            terms = wasp.getTerms('lb',a)
            if len(terms) != 2 or terms[1] != ID:
                continue
            if not lb is None:
                assert False     
            lb = int(terms[0])

    assert not lb is None
    
    # debug("lb", lb)

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
        mps = mps + weight[mw(G)]
    
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

    # print_I(I, atomNames, aggregate)
    # print_weights(weight, atomNames, aggregate)
    # print_groups(group, atomNames, aggregate)

    return bind 


def simplifyAtLevelZero():
    global N,lb, I, weight, aggregate, groups, group, reason

    # INCOHERENT
    if mps < lb:
        return [1]
    
    res = propagate_phase(None)
    
    simplyLiterals(lits_level_0, aggregate, group)

    return res

def update_phase(l: int) -> (bool, Group):
    global N,lb, I, weight, aggregate, groups, mps, group, true_group

    I[l] = True
    G : Group
    if aggregate[l]:
        G = group[l]
        G.decrease_und()
        # debug("true", get_name(atomNames, l), G = G)
        
        true_group[G] = l
        w_max = weight[mw(G)]
        mps = mps - w_max + weight[l]
        if w_max == weight[l]:
            return (False, G)
        
    elif aggregate[not_(l)]:
        G = group[not_(l)]
        G.decrease_und()
        # debug("false", get_name(atomNames, not_(l)), G = G)

        if not_(l) == mw(G):
            new_max, prev_max = G.update_max(I)
            if true_group[G] is None :
                w_n : int  = 0
                w_n = weight[new_max]

                w_p = weight[prev_max]
                mps = mps - w_p + w_n  
                if w_p == w_n:
                    return (False, None)
            else:
                return (False, G)   
        else:
            return (False, None)      
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

    for g in groups:

        ml_g =  mw(g)
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

        # infer the truth
        if g.count_undef - count_infered_falses == 1 and true_group[g] is None:
            # the last remained literal 
            l = mw(g)
            if mps - weight[l] < lb:
                S.append(l)
                reason_trues[l] = false_lits_g

    if len(S) != 0:
        for g in groups:
            # print_I(I, atomNames, aggregate)
            if g.count_undef == 0 and true_group[g] is None:
                R.extend(g.ord_l)
            elif true_group[g] is None:
                mw_g = weight[mw(g)]
                for i in range(len(g.ord_l) - 1, -1, -1):
                    l = g.ord_l[i]
                    if weight[l] <= mw_g:
                        break
                    R.append(l)
            else:
                R.append(not_(true_group[g]))

        # updating the reason
        reason_falses = R

        # debug("mps",mps, G = G)
        # for g in groups:
        #     if true_group[g]:
        #         debug(get_name(atomNames, true_group[g]), weight[true_group[g]] , "true", G = G, end= " ")
        #     else:
        #         debug(get_name(atomNames, mw(g)), weight[mw(g)], "undef", G = G, end= " ")
        #     debug("", G = G)

        # debug("S => ", G = G)
        # for s in S :
        #     debug(get_name(atomNames, s), G = G)
        # debug("END S", G = G)
        # debug("R: ", G = G)
        
        # for r in R :
        #     debug(get_name(atomNames, r), G = G)
        # debug("END R", G = G)


    return S

def onLiteralTrue(lit, dl):

    # debug("lit true", get_name(atomNames, lit))

    (next_phase, G) = update_phase(lit)

    propagated_lits = []
    if next_phase:
        propagated_lits = propagate_phase(G)

    return propagated_lits    


def onLiteralsUndefined(*lits):
    global N,lb, I, weight, aggregate, groups,  mps, group, reason, true_group
    for i in range(1,len(lits)):
        l = lits[i]

        # updating interpretation
        I[l] = None

        # print_I(I, atomNames, aggregate)

        # updating max weight for group(l)
        G : Group = group[l] 

        if G is None:
            G = group[not_(l)]
            l = not_(l)

        # increasing the number of undefined for G
        G.increase_und()

        # debug("undefined ", get_name(atomNames, l), G = G)

        tg = true_group[G]

        # the true literal of G became undefined
        if tg == l:
            true_group[G] = None

        max_und = mw(G)

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
            return
        
        pos_max = G.ord_i[max_und]
        pos_l   = G.ord_i[l]
        max_w = weight[mw(G)]
        
        mps_p = mps 
        if tg == l:

            # updating the mps
            if max_w > weight[l]: 
                mps = mps - weight[l] + max_w
                # debug(f"prev mps {mps_p} new mps {mps}")


            # updating the max undefined
            if pos_max < pos_l:
                G.set_max(l)

        else:
            w_l = weight[l]
            if w_l >= max_w and pos_l > pos_max:
                G.set_max(l)
                if tg is None:
                    mps = mps - max_w + w_l
                    # debug(f"prev mps {mps_p} new mps {mps}")

        

        


            

        

