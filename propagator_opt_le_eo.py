#!/usr/bin/python3
import wasp
from typing import List
from utility import PerfectHash, debug, Group, Interpretation, WeightFunction,\
    GroupFunction, min_w, not_, get_name, print_I, print_weights, print_groups, FOCUSED_GROUP, \
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
Propagator for ' <= UB ' constraint with Exactly One constraint 
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

# upper bound
ub : int

# min possible sum
mps : int

# literals derived at level 0
lits_level_0 : List[int] = []

reason : List[int] = []


def getReason():
    global reason
    return reason

def process_sys_parameters():
    global ID
    ID = sys_parameters.pop()
    sys_parameters.pop()

def getLiterals(*lits):
    global N,up, I, weight, aggregate, groups, mps, group, atomNames, true_group, lits_level_0, ID

    up = None
    bind = []
    negative_lit_regex = re.compile(r"^not\s+(?P<atom_name>[\w()]+)")
    ID = sys_parameters[-1]
    print(ID)
    print(sys_parameters)

    # initializing 
    N = lits[0] + 1
    I = Interpretation(N)
    weight = WeightFunction(N)
    group = GroupFunction(N)
    aggregate = AggregateFunction(N, False)
    mps = 0

    #used to create the groups
    groups_raw : dict[int, List[int]] = {}
    groups = set()
    print(f"parameters {sys_parameters}")

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
        
        elif a.startswith("up("):
            terms = wasp.getTerms('up',a)
            if len(terms) != 2 or terms[1] != ID:
                continue
            if not up is None:
                assert False     
            up = int(terms[0])

    assert not up is None
    
    debug("up", up)

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
        
        # updatind the min possible sum
        mps = mps + weight[min_w(G)]
    
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

    print_I(I, atomNames, aggregate)
    print_weights(weight, atomNames, aggregate)
    print_groups(group, atomNames, aggregate)

    return bind 


def simplifyAtLevelZero():
    global N,up, I, weight, aggregate, groups, group, reason

    # INCOHERENT
    if mps > up:
        return [1]
    
    res = propagate_phase(None)
    
    simplyLiterals(lits_level_0, aggregate, group)

    return res

def update_phase(l: int) -> (bool, Group):
    global N,up, I, weight, aggregate, groups, mps, group, true_group

    I[l] = True
    G : Group
    if aggregate[l]:
        G = group[l]
        G.decrease_und()
        # debug("true", get_name(atomNames, l), G = G)
        
        true_group[G] = l
        w_min = weight[min_w(G)]
        mps = mps - w_min + weight[l]
        if w_min == weight[l]:
            return (False, G)
        
    elif aggregate[not_(l)]:
        G = group[not_(l)]
        G.decrease_und()
        # debug("false", get_name(atomNames, not_(l)), G = G)

        if not_(l) == min_w(G):
            new_min, prev_min = G.update_min(I)
            if true_group[G] is None :
                w_n : int  = 0
                w_n = weight[new_min]
                w_p = weight[prev_min]
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
    global N,up, I, weight, aggregate, groups, mps, group, reason, true_group

    # set of derived literals
    S : List[int] = []
    
    # reason
    R : List[int] = []

    for g in groups:
        # name = get_name(atomNames, true_group[g]) if not true_group[g] is None else "none"
        # print(f" true group for {g.id} {name}")
        if not G is None and ( g == G or not true_group[g] is None ):
            continue

        ml_g =  min_w(g)
        mw_g = weight[ml_g]

        for i in range(len(g.ord_l)-1,-1,-1):
            l = g.ord_l[i]
            # print(get_name(atomNames, l))
            if I[l] is None:
                if mps - mw_g + weight[l] > up:
                    # infer l as false
                    S.append(not_(l))
                    g.decrease_und()
                else:
                    break

    
    if len(S) != 0:
        for g in groups:
            # print_I(I, atomNames, aggregate)
            if true_group[g] is None:
                mw_g = weight[min_w(g)]
                for l in g.ord_l:
                    if weight[l] >= mw_g:
                        break
                    R.append(l)
            else:
                R.append(not_(true_group[g]))
   
        # updating the reason
        reason = R

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
    global N,up, I, weight, aggregate, groups,  mps, group, reason, true_group
    for i in range(1,len(lits)):
        l = lits[i]

        # updating interpretation
        I[l] = None

        # print_I(I, atomNames, aggregate)

        # updating min weight for group(l)
        G = group[l]

        if G is None:
            G = group[not_(l)]
            l = not_(l)

        # debug("undefined ", get_name(atomNames, l), G = G)

        tg = true_group[G]

        # the true literal of G became undefined
        if tg == l:
            true_group[G] = None

        min_und = min_w(G)

        '''
        if G has all literals defined
            G.set_min(l)
            return

        if l was the true literal of the group 
   
            # updating the mps
            if weight[min_w(G)] < weight[l] -> mps = mps - weight[l] + weight[min_w(G)]
            
            # updating the min undefined
            if pos(min_w(G)) > pos(l)  -> G.set_min(l)

        else
        
            min_w = weight[min_w(G)]
            if weight[l] <= min_w and pos(l) < pos(min_w(G))
                G.set_min(l)
            
            if G has not true literal
                mps = mps - min_w + weight[min_w(G)]
        '''

        if min_und is None:
            G.set_min(l)
            return
        
        pos_min = G.ord_i[min_und]
        pos_l   = G.ord_i[l]
        min_w = weight[min_w(G)]
        
        mps_p = mps 
        if tg == l:

            # updating the mps
            if min_w < weight[l]: 
                mps = mps - weight[l] + min_w
                # debug(f"prev mps {mps_p} new mps {mps}")


            # updating the min undefined
            if pos_min < pos_l:
                G.set_min(l)

        else:
            w_l = weight[l]
            if w_l <= min_w and pos_l < pos_min:
                G.set_min(l)
                if tg is None:
                    mps = mps - min_w + w_l
                    # debug(f"prev mps {mps_p} new mps {mps}")

        

        


            

        

