#!/usr/bin/python3


import copy
from itertools import chain, combinations
import random
from typing import List

from utility import GroupFunction, WeightFunction



def get_all_lit_below_you(lits, l):

    res = []
    found = False
    for i in range(len(lits) - 1, -1, -1):
        lit = lits[i]
        if lit == l:
            found = True
        if found:
            res.append(lit)

    return res

def all_subsets(s):

    return chain.from_iterable(combinations(s, r) for r in range(len(s) + 1))

def correct_maximum_subset_sum_less_than_s_with_groups(s: int, literals : List[list], group: dict, weight: dict):

    set_lit = set(literals)
    subsets = all_subsets(set_lit)
    # subsets = list(subsets)
    maximum_subset = set()

    grouped_lits = {id: [] for _,id in group.values()}
    for subset in subsets:
        sum = 0
        grouped_lits_c = copy.deepcopy(grouped_lits)
        for l in subset:
            id = group[l][1]
            grouped_lits_c[id].append(l)
        for id in grouped_lits_c:
            lits_g = grouped_lits_c[id]
            if len(lits_g) > 0:
                sum += weight[max(lits_g, key= lambda x: weight[x])]
        # print("subset",subset,"sum",sum)
        if sum <= s and len(maximum_subset) < len(subset):
            maximum_subset = subset

    return maximum_subset
            

def maximum_subset_sum_less_than_s_with_groups(s: int, literals : List[list], group: dict, weight: dict):
    
    n = len(literals)
    subset = [[None for _ in range(n + 1)]
                    for _ in range (s + 1)]

    # If sum is 0, then the maximal subset is empty 
    for i in range (n + 1):
        subset[0][i] = set([])


    # NOTE: it is not necessary
    # # If sum is not 0 and set e is empty, 
    # # then the maximal subset does not exist
    # for i in range (1, sum + 1):
    #     subset[0][i] = None

    # Fill the subset table in bottom up manner 
    for i in range (1, s + 1): 
        for j in range (1, n + 1):
            lit = literals[j - 1]
            w = weight[lit]
            subset[i][j] = subset[i][j - 1]
            if (i >= w) :
                sum_got = not subset[i][j] is None or not subset[i - w ][j - 1] is None
                if (sum_got):
                    group_lits = group[lit][0]
                    with_lit_subset = subset[i - w][j - 1].union(set(get_all_lit_below_you(group_lits, l=lit))) if not subset[i - w][j - 1] is None else None
                    subset[i][j] = max( 
                            subset[i][j - 1], 
                            with_lit_subset,
                            key= lambda x: len(x) if not x is None else -1
                        )
                    
    maximum_subset = subset[s][n]

    print(f"\"X\"", end=",")
    print(f"\"N\"", end=",")

    for l in literals:
        print(f"\"{l}\"", end=",")
    print()
    for i in range (0, s + 1): 
        print(f"\"{i}\"", end=",")
        for j in range (0, n + 1):
            string = str(subset[i][j]) if not subset[i][j] is None else "/"
            print(f"\"{string}\"", end=",")
        print()

    len_subset_max = len(maximum_subset) if subset[i][n] else -1
    for i in range(s+1):
        len_subset_max = len(maximum_subset) if not maximum_subset is None else -1
        len_subset = len(subset[i][n]) if not subset[i][n] is None  else -1
        # print("len_subset_max", len_subset_max)
        # print("len_subset", len_subset)
        if len_subset > len_subset_max:
            maximum_subset = subset[i][n]
    
    return maximum_subset

def create_random_instance():

    # number of literals 
    n = random.randint(7, 20)
    literals = [i+1 for i in range(n)]

    weight = {}
    for l in literals:
        weight[l] = random.randint(0,10)

    # number of groups
    ng = random.randint(n//4, n//1.5)

    group = {i: [[],i] for i in range(ng)}
   
    for l in literals:
        gid = random.randint(0,ng-1)
        group[gid][0].append(l)
    
    group_lits = {}
    for gid in group:
        lits = group[gid][0]
        lits = sorted(lits, key=lambda x: weight[x])
        for l in lits:
            group_lits[l] = [lits,gid]
        group[gid][0] = lits

    lb = random.randint(7, 10)
    
    return (literals, lb, group_lits, weight)

def checkcorrectness(tries):
    for t in range(tries):
        literals, lb, group, weight = create_random_instance()
        
        # print("lb", lb)
        # print("literals", literals)
        # print("weight", weight)
        # print("group", group)

        maximum_subset_dp = maximum_subset_sum_less_than_s_with_groups(s = lb - 1 , literals=literals, 
                                                                    group=group, weight=weight )
        maximum_subset_exp =  correct_maximum_subset_sum_less_than_s_with_groups(s = lb - 1 , literals=literals, 
                                                                    group=group, weight=weight)
        print("try",t,"dp",maximum_subset_dp,"exp",maximum_subset_exp, len(maximum_subset_dp) == len(maximum_subset_exp))
        if len(maximum_subset_dp) != len(maximum_subset_exp):
            print("lb", lb)
            print("literals", literals)
            print("weight", weight)
            print("group", group)
def main():

    literals = [1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19]
    group_lits_1 = [1, 2] 
    group_lits_2 = [3, 4, 5] 
    group_lits_3 = [6, 7] 
    lb = 9
    group = {10: [[10], 0], 8: [[8, 1, 11], 1], 1: [[8, 1, 11], 1], 11: [[8, 1, 11], 1], 17: [[17, 15, 4], 2], 15: [[17, 15, 4], 2], 4: [[17, 15, 4], 2], 7: [[7, 19, 2, 18, 14], 3], 19: [[7, 19, 2, 18, 14], 3], 2: [[7, 19, 2, 18, 14], 3], 18: [[7, 19, 2, 18, 14], 3], 14: [[7, 19, 2, 18, 14], 3], 3: [[3, 5, 12, 13, 6, 9, 16], 4], 5: [[3, 5, 12, 13, 6, 9, 16], 4], 12: [[3, 5, 12, 13, 6, 9, 16], 4], 13: [[3, 5, 12, 13, 6, 9, 16], 4], 6: [[3, 5, 12, 13, 6, 9, 16], 4], 9: [[3, 5, 12, 13, 6, 9, 16], 4], 16: [[3, 5, 12, 13, 6, 9, 16], 4]}
    weight = {1: 4, 2: 6, 3: 2, 4: 7, 5: 2, 6: 3, 7: 5, 8: 1, 9: 4, 10: 4, 11: 4, 12: 2, 13: 2, 14: 10, 15: 6, 16: 5, 17: 4, 18: 7, 19: 5}

    # groups_lits = [ 
    # [8, 1, 11],
    # [17, 15, 4],
    # [10],
    # [7, 19, 2, 18, 14],
    # [3, 5, 12, 13, 6, 9, 16]]

    # literals = []
    # for array in groups_lits:
    #     literals += array

    maximum_subset_dp = maximum_subset_sum_less_than_s_with_groups(s = lb - 1 , literals=literals, 
                                                                    group=group, weight=weight )
    # maximum_subset_exp =  correct_maximum_subset_sum_less_than_s_with_groups(s = lb - 1 , literals=literals, 
    #                                                             group=group, weight=weight)
    # print("dp",maximum_subset_dp,"exp",maximum_subset_exp, len(maximum_subset_dp) == len(maximum_subset_exp))
    print(maximum_subset_dp)
    # tries = 100
    # checkcorrectness(tries=tries)
    
if __name__ == "__main__":
    main()