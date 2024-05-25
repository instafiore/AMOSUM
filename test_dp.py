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

    return frozenset(res)

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
        subset[0][i] = set([ frozenset([]) ])


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
            if (i >= w and not subset[i - w][j - 1] is None) :
                if subset[i][j] is None:
                    subset[i][j] = set() 
                group_lits = group[lit][0]
                added_subsets = set()
                for sub in subset[i - w][j - 1]:
                    with_lit_subset = sub | (get_all_lit_below_you(group_lits, l=lit))
                    added_subsets.add(with_lit_subset)
                subset[i][j] = subset[i][j].union(added_subsets)
                


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

    maximum_subset = None
    len_subset_max = -1
    for i in range(s+1):
        if subset[i][n] is None:
            continue
        for sub in subset[i][n]:
            print(sub)
            len_subset_max = len(maximum_subset) if not maximum_subset is None else -1
            len_subset = len(sub) if not sub is None  else -1
            # print("len_subset_max", len_subset_max)
            # print("len_subset", len_subset)
            if len_subset > len_subset_max:
                maximum_subset = sub
    
    return maximum_subset

def maximum_subset_sum_less_than_s_with_groups_without_memory(s: int, literals : List[list], group: dict, weight: dict):
    
    n = len(literals)
    subset = [[None for _ in range(n + 1)]
                    for _ in range (s + 1)]

    # If sum is 0, then the maximal subset is empty 
    for i in range (n + 1):
        subset[0][i] = set([])
        if i == 0: 
            continue
        l = literals[i-1]
        cell = set([l]) if weight[l] == 0 else set()
        subset[0][i] = subset[0][i-1].union(cell)



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

    # print(f"\"X\"", end=",")
    # print(f"\"N\"", end=",")

    # for l in literals:
    #     print(f"\"{l}\"", end=",")
    # print()
    # for i in range (0, s + 1): 
    #     print(f"\"{i}\"", end=",")
    #     for j in range (0, n + 1):
    #         string = str(subset[i][j]) if not subset[i][j] is None else "/"
    #         print(f"\"{string}\"", end=",")
    #     print()

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
    n = random.randint(17, 23)
    literals = [i+1 for i in range(n)]

    weight = {}
    for l in literals:
        weight[l] = random.randint(0,10)

    # number of groups
    ng = random.randint(n//4, n//1.5)

    group_dict = {i: [[],i] for i in range(ng)}
   
    for l in literals:
        gid = random.randint(0,ng-1)
        group_dict[gid][0].append(l)
    
    group = {}
    for gid in group_dict:
        lits = group_dict[gid][0]
        lits = sorted(lits, key=lambda x: weight[x])
        for l in lits:
            group[l] = [lits,gid]
        group_dict[gid][0] = lits

    lb = n * random.randint(2, 4)

    grouped_dict = {id: [] for _,id in group_dict.values()}
    for l in literals:
        id = group[l][1]
        grouped_dict[id].append(l)
    for gid in grouped_dict:
        lits = grouped_dict[gid]
        lits = sorted(lits, key=lambda x: weight[x], reverse=True)
        grouped_dict[gid] = lits

    literals = []
    for gid in grouped_dict:
        lits = grouped_dict[gid]
        literals += lits

    # print(grouped_dict)
    
    return (literals, lb, group, weight)

def checkcorrectness(tries):
    for t in range(tries):
        literals, lb, group, weight = create_random_instance()
        
        # print("lb", lb)
        # print("literals", literals)
        # print("weight", weight)
        # print("group", group)

        maximum_subset_dp = maximum_subset_sum_less_than_s_with_groups_without_memory(s = lb - 1 , literals=literals, group=group, weight=weight )
        print("DP FINISHED")
        maximum_subset_exp =  correct_maximum_subset_sum_less_than_s_with_groups(s = lb - 1 , literals=literals,  group=group, weight=weight)
        print("EXP FINISHED")
        len_maximum_subset_dp = len(maximum_subset_dp) if not maximum_subset_dp is None else -1
        len_maximum_subset_exp = len(maximum_subset_exp) if not maximum_subset_exp is None else -1
        print("try",t,"dp",maximum_subset_dp,"exp",maximum_subset_exp,  len_maximum_subset_dp == len_maximum_subset_exp)
        if len(maximum_subset_dp) != len(maximum_subset_exp):
            print("lb", lb)
            print("literals", literals)
            print("weight", weight)
            print("group", group)


def create_settings_from_failed_run():

    lb = 9
    literals = [1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11]
    weight = {1: 3, 2: 5, 3: 5, 4: 10, 5: 4, 6: 10, 7: 0, 8: 3, 9: 2, 10: 3, 11: 6}
    group = {7: [[7, 9, 1, 8, 10, 3], 0], 9: [[7, 9, 1, 8, 10, 3], 0], 1: [[7, 9, 1, 8, 10, 3], 0], 8: [[7, 9, 1, 8, 10, 3], 0], 10: [[7, 9, 1, 8, 10, 3], 0], 3: [[7, 9, 1, 8, 10, 3], 0], 5: [[5, 2, 11, 4, 6], 1], 2: [[5, 2, 11, 4, 6], 1], 11: [[5, 2, 11, 4, 6], 1], 4: [[5, 2, 11, 4, 6], 1], 6: [[5, 2, 11, 4, 6], 1]}

    grouped_dict = {id: [] for _,id in group.values()}
    for l in literals:
        id = group[l][1]
        grouped_dict[id].append(l)
    for gid in grouped_dict:
        lits = grouped_dict[gid]
        lits = sorted(lits, key=lambda x: weight[x], reverse=True)
        grouped_dict[gid] = lits

    literals = []
    for gid in grouped_dict:
        lits = grouped_dict[gid]
        literals += lits

    # print(grouped_dict)
    
    return (literals, lb, group, weight)

def main():

    # literals, lb, group, weight = create_settings_from_failed_run()
    # maximum_subset_dp = maximum_subset_sum_less_than_s_with_groups_without_memory(s = lb - 1 , literals=literals, group=group, weight=weight )
    # print(maximum_subset_dp)
    # maximum_subset_exp =  correct_maximum_subset_sum_less_than_s_with_groups(s = lb - 1 , literals=literals, group=group, weight=weight)
    # print("dp",maximum_subset_dp,"exp",maximum_subset_exp, len(maximum_subset_dp) == len(maximum_subset_exp))

    tries = 100
    checkcorrectness(tries)
    
if __name__ == "__main__":
    main()