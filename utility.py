# utility module

import re
import sys
from typing import Any, List

FOCUSED_GROUP = 2
FOCUSING = False

DEBUG = True

SEPARATOR = ":"
NOT = "~"
REGEX_LIT = rf"{NOT}?(\w+(\(\w+({SEPARATOR}\w+)*\))?)" 
REGEX_ASSUMPTIONS = rf"^\[{REGEX_LIT}({SEPARATOR}{NOT}?{REGEX_LIT})*\]$"
VALID_VALUES_ASS = f"[[{NOT}]<atom_name>[(param1,parm2,...)]:...] "

def debug(*message: str, G: 'Group' = None , end ="\n"):
    if DEBUG and ( G is None or G.id == FOCUSED_GROUP or not FOCUSING):
        print(message, end=end, file=sys.stderr)

def create_assumptions_lits(assumptions, atomNames):

    if not assumptions:
        return []

    res = []
    
    r2 = rf"^{NOT}.+$" 
    for ass in assumptions:
        atom = re.match(REGEX_LIT,ass).group(1)
        if not atom in atomNames:
            continue
        lit = atomNames[atom]
        if re.match(r2,ass):
            lit *= -1
        res.append(lit)

    return res

def convert_assparam_to_assarray(assumptions):
 
    # Strip the square brackets
    stripped_string = assumptions.strip("[]")

    # Split the string by comma
    array = stripped_string.split(SEPARATOR)

    # If there might be extra spaces around the elements, you can also strip each element
    array = [element.strip() for element in array]

    return array

class SymmetricFunction:
    
    def __init__(self, N) -> None:
        # interpretation
        self.intepretation : List[Any] = [None] * N

    def __getitem__(self, lit: int) -> Any:
        i = abs(lit) 
        value = self.intepretation[i]
        if lit < 0:
            value = not value
        return value
    
    def __setitem__(self, lit: int, value: Any):
        i = abs(lit)
        if lit < 0 :
            if not value is None:
                value = not value
            else:
                value = None
        self.intepretation[i] = value

class WeightFunction(SymmetricFunction):
    
    def __getitem__(self, lit: int) -> any:
        if lit is None:
            return 0
        return super().__getitem__(lit)


class Group:
    
    autoincrement = 0 

    def __init__(self, ord_l, ord_i, id) -> None:
        # number of literals
        self.N = len(ord_l)

        # number of undefined literals
        self.count_undef = self.N

        #  ord_l[i] = l 
        #  is the i-th literal orderded by weight
        self.ord_l : List[int] = ord_l

        # a function literals -> int that takes in input the literal l and gives as output the index of a in ord_lit:
        #   ord_i[l] = i 
        #   it is the inverse of ord_l function
        self.ord_i : dict[int, int] = ord_i

        # It is the index of the maximum (weight) undefined literal in ord_l 
        self.max_und : int = self.N - 1

        # It is the index of the minimum (weight) undefined literal in ord_l 
        self.min_und : int = 0 
        
        # all false literals of the group
        self.falses : List[int] = []

        # id of the group
        self.id = id
        self.id_autoinc = Group.autoincrement 
        Group.autoincrement += 1

    def increase_und(self):
        self.count_undef += 1
    
    def decrease_und(self):
        self.count_undef -= 1

    def add_false_lit(self, lit: int):
        self.falses.append(lit)

    def remove_false_lit(self, lit: int):
        self.falses.remove(lit)

    def set_max(self, l: int):
        self.max_und = self.ord_i[l]

    def set_min(self, l: int):
        self.min_und = self.ord_i[l]

    def update_max(self, I: SymmetricFunction, all = False):
        start = self.max_und - 1
        prev_max = self.ord_l[self.max_und]
        
        # All are defined
        if start < 0:
            self.max_und = None
            return (None, prev_max)
        
        if all:
            start = self.N - 1
        for i in range(start, -1, -1):
            l = self.ord_l[i]
            if I[l] is None:
                self.max_und = i
                new_max = self.ord_l[self.max_und]
                return (new_max, prev_max)
        
        # All are defined
        self.max_und = None
        return (None, prev_max)
    
    def update_min(self, I: SymmetricFunction, all = False):
        start = self.min_und + 1
        prev_min = self.ord_l[self.min_und]
        
        # All are defined
        if start >= self.N:
            self.min_und = None
            return (None, prev_min)
        
        if all:
            start = 0
        for i in range(start, self.N, +1):
            l = self.ord_l[i]
            if I[l] is None:
                self.min_und = i
                new_min = self.ord_l[self.min_und]
                return (new_min, prev_min)
        
        # All are defined
        self.min_und = None
        return (None, prev_min)
        

    def print_group(self, atomNames):
        lit_names = ""
        for l in self.ord_l:
            lit_names += " " + get_name(atomNames,l) + " "
        debug(str(self), f"[{lit_names}]")

    def __str__(self) -> str:
        return str(self.id)

# removes useless literals
def simplyLiterals(lits, aggregate: 'AggregateFunction', group: 'GroupFunction'):
    G : Group = None
    for l in lits:
        if aggregate[l]:
            G = group[l]
        elif aggregate[not_(l)]:
            G = group[not_(l)]
            l = not_(l)
        else:
            continue
        G.decrease_und()
        G.ord_l.remove(l)

# This function returns the max UNDEFINED literal
def max_w(g: Group):
    max_und = g.max_und
    if(max_und is None):
        return None
    return g.ord_l[max_und]


# This function returns the min UNDEFINED literal
def min_w(g: Group):
    min_und = g.min_und
    if(min_und is None):
        return None
    return g.ord_l[min_und]

def not_(l: int):
    return l * -1

def remove_elements(original, to_remove):
    return [element for element in original if element not in to_remove]

class PerfectHash:

    def __getitem__(self, lit: int) -> Any:
        if lit > 0:
            i = lit
        else:
            i = abs(lit) + self.N
        return self.values[i]
    
    def __setitem__(self, lit, value) -> None:
        if lit > 0:
            i = lit
        else:
            i = abs(lit) + self.N
        self.values[i] = value

    def __init__(self, N: int,  default = None) -> None:
        # it is a (N * 2) vector where:
        #   values[:N-1]    are the values for the positive literals
        #   values[N:]      are the values for the negative literals
        self.values = [default] * (N * 2)
        self.N = N

class AggregateFunction(PerfectHash):
    pass

class GroupFunction(PerfectHash):
    
    def __getitem__(self, lit: int) -> Group:
        return super().__getitem__(lit)

class TrueGroupFunction(PerfectHash):
    
    def __setitem__(self, group: Group, value) -> None:
        autoincrement = group.id_autoinc
        return super().__setitem__(autoincrement, value)
    
    def __getitem__(self, group: Group) -> any:
        autoincrement = group.id_autoinc
        return super().__getitem__(autoincrement)

# utility function for debugging
def get_name(atomNames, lit):
    if not DEBUG:
        return "DEBUG FALSE"
    prefix = ""
    if lit is None:
        return "None"
    if lit < 0:
        prefix = "not "
    for a in atomNames:
        if atomNames[a] == abs(lit):
            return prefix + a
    debug(f"{lit}")

def convert_array_to_string(name, array, atomNames, array_of_lits = True):

    res = ""
    res += f"{name} [ "
    for l in array:
        l = get_name(atomNames=atomNames, lit = l) if array_of_lits else l
        res+= f"{l} "
    res += "]"
    return res

def print_I(I, atomNames, aggregate, G = None, group = None):
    if not DEBUG:
        return
    assert (G is None and group is None) or (not G is None and not group is None) 
    if G is None:
        debug("Interpretation", end=" ")
    else:
        debug("Intepretation for group: " + str(G), end=" ")
    for l in range(len(I.intepretation)):
        if (aggregate[l] or  aggregate[not_(l)]) and (G is None or group[l] == G):
            debug(get_name(atomNames,l), I[l],end=" ")
    debug("")

def print_perfect_hash(ph: PerfectHash, atomNames, aggregate: AggregateFunction):
    if not DEBUG:
        return
    
    for i in range(len(ph.values)):
        l = i
        if l >= ph.N:
            l = not_(l - ph.N)
        if aggregate[l] or aggregate[not_(l)]:
            debug(get_name(atomNames,l), str(ph[l]))

def print_weights(weight: WeightFunction, atomNames, aggregate: AggregateFunction):
    debug("Weights")
    print_perfect_hash(weight, atomNames, aggregate)

def print_groups(group: GroupFunction, atomNames, aggregate):
    debug("Groups")
    print_perfect_hash(group, atomNames, aggregate)

def get_increment_name(increment: dict, atomNames: dict):
    increment_name = {}
    for i in increment:
        increment_name[f"{get_name(atomNames=atomNames,lit=i)}"] = increment[i]
    return increment_name

# MINIMIZING REASON 
#################################################################################################################################################
    

def is_true_in_reason(lit, group: GroupFunction):
    '''
    invariants: the literal is in the reason and it has been flipped (if true)
    '''
    g = group[lit]
    return g is None

# TODO: remove the parameter true_group
def increment_f(l: int, current_subset_maximal, true_group: TrueGroupFunction, weight: WeightFunction, group: GroupFunction):
    g = group[l] 
    tr = False # true_in_reason
    if g is None:
        l = not_(l)
        g = group[l]
        tr = True # it means that it has been flipped, so it is true in reason (is the true_group[g])
        assert l == true_group[g]

    w = weight[l]
    if tr:  
        w_mw_g = weight[g.ord_l[-1]]
        return w_mw_g - w
    else:
        # i = g.ord_i[l]
        i = len(g.ord_i) - 1
        mw_g = weight[max_w(g)]
        current_l = l
        increment = w - mw_g
        while mw_g < weight[current_l]:
            if current_l in current_subset_maximal:
                increment = max(0,w - weight[current_l])
                # if it 0 means that in the current minimal subset is already present some literal of the same group
                #  but higher weight
                break
            i -= 1
            current_l = g.ord_l[i]
        return increment

# TODO: remove the parameter true_group and atomNames
def compute_minimal_reason(reason: List[int], trues: List[int], weight: WeightFunction, true_group: TrueGroupFunction, group: Group, atomNames: dict):
 
    # computing increment for each literal 
    increment = compute_increment_literals(literals=reason)
    debug(f"increment {get_increment_name(increment=increment)}")
    
    for l in trues:
        reason_l_to_minimize = []
        g = group[l]
        assert not g is None
        s = lb - (mps - weight[l]) - 1 
        for lr in reason:
            glr = group[lr] 
            # it means that the literal is true, since for sure has been flipped (it has not group otherwise)
            # so cannot be the same group of l, since l is also true 
            if not glr is None and g.id == glr.id:
                continue
            reason_l_to_minimize.append(l)

        redundant_lits[l] = maximal_subset_sum_with_groups(literals=reason_l_to_minimize, s = s, w= weight, true_group=true_group, group=group)
        # TODO: REMOVE
        if len(redundant_lits[l]) != 0: 
            redundant_lits_str = convert_array_to_string(name=f"redundant lits for {get_name(atomNames=atomNames,lit=l)}", array=redundant_lits[l], atomNames=atomNames)
            debug(redundant_lits_str)

def maximal_subset_sum_with_groups(literals: List[int], s: int, w: WeightFunction, true_group : TrueGroupFunction, group: GroupFunction):

    current_subset_maximal = []
    current_sum = 0

    for l in literals:
        inc = increment_f(l, current_subset_maximal, true_group, w, group)
        if current_sum + inc <= s:
                increment_l += inc
                current_subset_maximal.append(l)
    
    return current_subset_maximal

def compute_increment_literals(literals):
    increment = {}
    for l in literals:
        g : Group
        g = group[l] 
        if G is None:
            G = group[not_(l)]
            # l = not_(l)
        assert not g is None 
        mw_g = max_w(g)
        w_mw_g = weight[mw_g]
        w = weight[l]
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

#################################################################################################################################################
# END MINIMIZING REASON