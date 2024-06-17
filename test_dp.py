#!/usr/bin/python3


import copy
from itertools import chain, combinations
import random
from typing import List

from utility import GroupFunction, WeightFunction



def get_all_lit_below_you(lits, l, weight):

    res = []
    found = False
    for i in range(len(lits) - 1, -1, -1):
        lit = lits[i]
        if weight[lit] == weight[l]:
            found = True
        if found:
            res.append(lit)

    return set(res)

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
            

def maximum_subset_sum_less_than_s_with_groups_unaltrochenonsochecifacciaqui(s: int, literals : List[list], group: dict, weight: dict):
    
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

def compute_sum(subset, group, weight):
    sum = 0
    grouped_lits = {id: [] for _,id in group.values()}
    for l in subset:
        id = group[l][1]
        grouped_lits[id].append(l)
    for id in grouped_lits:
        lits_g = grouped_lits[id]
        if len(lits_g) > 0:
            sum += weight[max(lits_g, key= lambda x: weight[x])]

    return sum

def maximum_subset_sum_less_than_s_with_groups(s: int, literals : List[list], group: dict, weight: dict):
    
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
                    correct_subset = True
                    k = j
                    while True:
                        if not subset[i - w][k - 1] is None:
                            if not lit in subset[i - w][k - 1]:
                                break
                        else:
                            correct_subset = False
                            break
                        k -= 1
                        if k <= 0:
                            break
                    with_lit_subset = subset[i - w][k - 1].union(set(get_all_lit_below_you(group_lits, l=lit, weight=weight))) if correct_subset else None
                    subset[i][j] = max( 
                            subset[i][j - 1], 
                            with_lit_subset,
                            key= lambda x: len(x) if not x is None else -1
                        )
                    
    maximum_subset = subset[s][n]

    # print(f"\"W\"", end=",")
    # print(f"\"N\"", end=",")
    # for l in literals:
    #     print(f"\"{group[l][1]}:{weight[l]}\"", end=",")
    # print()

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
    n = random.randint(15, 25)
    literals = [i+1 for i in range(n)]

    weight = {}
    for l in literals:
        weight[l] = random.randint(0,30)

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

    lb = n * random.randint(1, 2)

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

        maximum_subset_dp = maximum_subset_sum_less_than_s_with_groups(s = lb - 1 , literals=literals, group=group, weight=weight )
        maximum_subset_exp =  correct_maximum_subset_sum_less_than_s_with_groups(s = lb - 1 , literals=literals,  group=group, weight=weight)
        len_maximum_subset_dp = len(maximum_subset_dp) if not maximum_subset_dp is None else -1
        len_maximum_subset_exp = len(maximum_subset_exp) if not maximum_subset_exp is None else -1
        print("try",t,"dp",maximum_subset_dp,"exp",maximum_subset_exp,  len_maximum_subset_dp == len_maximum_subset_exp)
        if len(maximum_subset_dp) != len(maximum_subset_exp):
            print("lb=", lb)
            print("literals=", literals)
            print("weight=", weight)
            print("group=", group)


def create_settings_from_failed_run():

    lb = 24535 + 1
    literals = [53, 51, 49, 47, 45, 43, 41, 39, 37, 35, 33, 31, 29, 27, 25, 23, 21, 93, 91, 89, 87, 85, 83, 81, 79, 77, 75, 73, 71, 69, 67, 65, 63, 61, 59, 57, 55]
    weight = {53: 20376, 51: 19244, 49: 18112, 47: 16980, 45: 15848, 43: 14716, 41: 13584, 39: 12452, 37: 11320, 35: 10188, 33: 9056, 31: 7924, 29: 6792, 27: 5660, 25: 4528, 23: 3396, 21: 2264, 93: 30600, 91: 29070, 89: 27540, 87: 26010, 85: 24480, 83: 22950, 81: 21420, 79: 19890, 77: 18360, 75: 16830, 73: 15300, 71: 13770, 69: 12240, 67: 10710, 65: 9180, 63: 7650, 61: 6120, 59: 4590, 57: 3060, 55: 1530}
    group = {53: [[15, 17, 19, 21, 23, 25, 27, 29, 31, 33, 35, 37, 39, 41, 43, 45, 47, 49, 51, 53], 1], 51: [[15, 17, 19, 21, 23, 25, 27, 29, 31, 33, 35, 37, 39, 41, 43, 45, 47, 49, 51, 53], 1], 49: [[15, 17, 19, 21, 23, 25, 27, 29, 31, 33, 35, 37, 39, 41, 43, 45, 47, 49, 51, 53], 1], 47: [[15, 17, 19, 21, 23, 25, 27, 29, 31, 33, 35, 37, 39, 41, 43, 45, 47, 49, 51, 53], 1], 45: [[15, 17, 19, 21, 23, 25, 27, 29, 31, 33, 35, 37, 39, 41, 43, 45, 47, 49, 51, 53], 1], 43: [[15, 17, 19, 21, 23, 25, 27, 29, 31, 33, 35, 37, 39, 41, 43, 45, 47, 49, 51, 53], 1], 41: [[15, 17, 19, 21, 23, 25, 27, 29, 31, 33, 35, 37, 39, 41, 43, 45, 47, 49, 51, 53], 1], 39: [[15, 17, 19, 21, 23, 25, 27, 29, 31, 33, 35, 37, 39, 41, 43, 45, 47, 49, 51, 53], 1], 37: [[15, 17, 19, 21, 23, 25, 27, 29, 31, 33, 35, 37, 39, 41, 43, 45, 47, 49, 51, 53], 1], 35: [[15, 17, 19, 21, 23, 25, 27, 29, 31, 33, 35, 37, 39, 41, 43, 45, 47, 49, 51, 53], 1], 33: [[15, 17, 19, 21, 23, 25, 27, 29, 31, 33, 35, 37, 39, 41, 43, 45, 47, 49, 51, 53], 1], 31: [[15, 17, 19, 21, 23, 25, 27, 29, 31, 33, 35, 37, 39, 41, 43, 45, 47, 49, 51, 53], 1], 29: [[15, 17, 19, 21, 23, 25, 27, 29, 31, 33, 35, 37, 39, 41, 43, 45, 47, 49, 51, 53], 1], 27: [[15, 17, 19, 21, 23, 25, 27, 29, 31, 33, 35, 37, 39, 41, 43, 45, 47, 49, 51, 53], 1], 25: [[15, 17, 19, 21, 23, 25, 27, 29, 31, 33, 35, 37, 39, 41, 43, 45, 47, 49, 51, 53], 1], 23: [[15, 17, 19, 21, 23, 25, 27, 29, 31, 33, 35, 37, 39, 41, 43, 45, 47, 49, 51, 53], 1], 21: [[15, 17, 19, 21, 23, 25, 27, 29, 31, 33, 35, 37, 39, 41, 43, 45, 47, 49, 51, 53], 1], 93: [[55, 57, 59, 61, 63, 65, 67, 69, 71, 73, 75, 77, 79, 81, 83, 85, 87, 89, 91, 93], 2], 91: [[55, 57, 59, 61, 63, 65, 67, 69, 71, 73, 75, 77, 79, 81, 83, 85, 87, 89, 91, 93], 2], 89: [[55, 57, 59, 61, 63, 65, 67, 69, 71, 73, 75, 77, 79, 81, 83, 85, 87, 89, 91, 93], 2], 87: [[55, 57, 59, 61, 63, 65, 67, 69, 71, 73, 75, 77, 79, 81, 83, 85, 87, 89, 91, 93], 2], 85: [[55, 57, 59, 61, 63, 65, 67, 69, 71, 73, 75, 77, 79, 81, 83, 85, 87, 89, 91, 93], 2], 83: [[55, 57, 59, 61, 63, 65, 67, 69, 71, 73, 75, 77, 79, 81, 83, 85, 87, 89, 91, 93], 2], 81: [[55, 57, 59, 61, 63, 65, 67, 69, 71, 73, 75, 77, 79, 81, 83, 85, 87, 89, 91, 93], 2], 79: [[55, 57, 59, 61, 63, 65, 67, 69, 71, 73, 75, 77, 79, 81, 83, 85, 87, 89, 91, 93], 2], 77: [[55, 57, 59, 61, 63, 65, 67, 69, 71, 73, 75, 77, 79, 81, 83, 85, 87, 89, 91, 93], 2], 75: [[55, 57, 59, 61, 63, 65, 67, 69, 71, 73, 75, 77, 79, 81, 83, 85, 87, 89, 91, 93], 2], 73: [[55, 57, 59, 61, 63, 65, 67, 69, 71, 73, 75, 77, 79, 81, 83, 85, 87, 89, 91, 93], 2], 71: [[55, 57, 59, 61, 63, 65, 67, 69, 71, 73, 75, 77, 79, 81, 83, 85, 87, 89, 91, 93], 2], 69: [[55, 57, 59, 61, 63, 65, 67, 69, 71, 73, 75, 77, 79, 81, 83, 85, 87, 89, 91, 93], 2], 67: [[55, 57, 59, 61, 63, 65, 67, 69, 71, 73, 75, 77, 79, 81, 83, 85, 87, 89, 91, 93], 2], 65: [[55, 57, 59, 61, 63, 65, 67, 69, 71, 73, 75, 77, 79, 81, 83, 85, 87, 89, 91, 93], 2], 63: [[55, 57, 59, 61, 63, 65, 67, 69, 71, 73, 75, 77, 79, 81, 83, 85, 87, 89, 91, 93], 2], 61: [[55, 57, 59, 61, 63, 65, 67, 69, 71, 73, 75, 77, 79, 81, 83, 85, 87, 89, 91, 93], 2], 59: [[55, 57, 59, 61, 63, 65, 67, 69, 71, 73, 75, 77, 79, 81, 83, 85, 87, 89, 91, 93], 2], 57: [[55, 57, 59, 61, 63, 65, 67, 69, 71, 73, 75, 77, 79, 81, 83, 85, 87, 89, 91, 93], 2], 55: [[55, 57, 59, 61, 63, 65, 67, 69, 71, 73, 75, 77, 79, 81, 83, 85, 87, 89, 91, 93], 2]}


    
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
    literals = [1,2,5,4,3,]
    return (literals, lb, group, weight)

def main():

    # literals, lb, group, weight = create_settings_from_failed_run()
    
    # print(compute_sum({1, 2, 4, 6, 7, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 20, 21, 22}, group, weight))
    
    subset_failed = {1, 2, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 15, 16, 17, 18, 19, 20, 21}    
    # print(group_lits)
    # print(compute_sum(subset_failed, group, weight))
    
    # maximum_subset_dp = maximum_subset_sum_less_than_s_with_groups(s = lb - 1 , literals=literals, group=group, weight=weight )
    # # print(maximum_subset_dp)
    # maximum_subset_exp =  correct_maximum_subset_sum_less_than_s_with_groups(s = lb - 1 , literals=literals, group=group, weight=weight)
    # print("dp",maximum_subset_dp,"exp",maximum_subset_exp, len(maximum_subset_dp) == len(maximum_subset_exp))

    tries = 1000
    checkcorrectness(tries)
    
if __name__ == "__main__":
    main()