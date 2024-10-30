#!/home/s.fiorentino/miniconda3/bin/python3
# utility module
from enum import Enum
import re
import sys
from typing import Any, List
import sys
# import clingo 

# Debug mode
DEBUG = False

# focusing print for a particular group
FOCUSED_GROUP = 2
FOCUSING = False

# assumptions
SEPARATOR = ":"
NOT = "~"
REGEX_LIT = rf"{NOT}?(\w+(\(\w+({SEPARATOR}\w+)*\))?)" 
REGEX_ASSUMPTIONS = rf"^\[{REGEX_LIT}({SEPARATOR}{NOT}?{REGEX_LIT})*\]$"
VALID_VALUES_ASS = f"[[{NOT}]<atom_name>[(param1,parm2,...)]:...] "

def set_debug(debug):
    global DEBUG
    DEBUG = debug if debug != "" else DEBUG

def print_err(*message: str, end ="\n"):
    print(message, end=end, file=sys.stderr)

# clingo.PropagateControl
def print_propagate(propagator, changes: List[int], control = None, dl = 0, force_print = False, wasp_b = False):
    if not force_print and not DEBUG:
        return 
    changes_str  = propagator.compute_changes_str(changes=changes, thread_id=control.thread_id) if not wasp_b else \
        f"[{get_name(lit=changes[0], atomNames=propagator.atomNames)}, {changes[0]}]"
    decision_slit = control.assignment.decision(dl) if not wasp_b else propagator.last_decision_lit
    plit: int = 0
    if not wasp_b and decision_slit != 1:
        plit = propagator.map_slit_plit_watched[decision_slit][0]
    elif wasp_b:
        plit = propagator.last_decision_lit 
    decision_literal_name = get_name(atomNames = propagator.atomNames, lit = plit) if decision_slit != 1 else "from facts"
    debug(f"[{decision_literal_name}, {dl}] propagate {changes_str} thread_id: {control.thread_id if not wasp_b else 0}", force_print=force_print)

def print_clause(propagator, clause, force_print = False, conflict = False):
    if not force_print and not DEBUG:
        return 
    clause_names = [get_name(atomNames=propagator.atomNames, lit=propagator.map_slit_plit_watched[literal][0]) for literal in clause]
    confict_str = "Conflict with " if conflict else ""
    debug(f"{confict_str}clause_names {clause_names} clause_slits {clause}", force_print=force_print)

def equals(l1, l2):
    return abs(l1) == abs(l2)

def print_undo(propagator, changes, thread_id, force_print = False, wasp_b = False):
    if not force_print and not DEBUG:
        return 
    if wasp_b:
        changes = changes[1::]
    changes_str = propagator.compute_changes_str(changes=changes, thread_id=thread_id)
    debug(f"undo {changes_str} thread_id: {thread_id}", file = sys.stderr, force_print=force_print)


def print_derivation(atomNames, S, force_print = False):
    if not force_print and not DEBUG:
            return 
    S_str = convert_array_to_string(name="Derived", array=S, atomNames=atomNames)
    debug(S_str, file=sys.stderr, force_print=force_print)

def print_reason(atomNames, R, literal, force_print = False):
    if not force_print and not DEBUG:
            return 
    R_str = convert_array_to_string(name=f"Reason({get_name(atomNames=atomNames, lit=literal)})", array=R, atomNames=atomNames)
    debug(R_str, file=sys.stderr, force_print=force_print)

def print_reduction_reason(propagator, reason_c, reason, lit, force_print = False):
    if not force_print and not DEBUG:
        return 
    propagator.redundant_lits_str = convert_array_to_string(name=f"from {len(reason_c)} to {len(reason)} removed from reason of {get_name(atomNames=propagator.atomNames, lit=lit)} lit {lit}", array=propagator.redundant_lits[lit], atomNames=propagator.atomNames)
    debug(propagator.redundant_lits_str, force_print=force_print)
    propagator.redundant_lits_str = convert_array_to_string(name=f"removed from reason of {get_name(atomNames=propagator.atomNames, lit=lit)} ", array=propagator.redundant_lits[lit], atomNames=propagator.atomNames)
    debug(propagator.redundant_lits_str, force_print=force_print)

def debug(*message: str, G: 'Group' = None , end ="\n", force_print = False, file = sys.stderr):
    if force_print or (DEBUG and ( G is None or G.id == FOCUSED_GROUP or not FOCUSING)):
        print(message, end=end, file=file)

def init_run(argv):
    param = {}
    regex = r"^-(.+)" 
    
    i = 1
    while i < len(argv):
        
        # creating the key
        key = argv[i] 
        res_regex = re.match(regex, key)
        if res_regex is None:
            raise Exception("Every key has to start with a dash! Ex: -problem knapsack")
        key = res_regex.group(1)

        if i + 1 >= len(argv) :
            param[key] = True
            break
        
        value = argv[i+1] 
        res_regex = re.match(regex, value)
        if res_regex is None:
            i += 2
            param[key] = value
            
        else:
            i += 1
            param[key] = True

    return param

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

def process_sys_parameters(sys_parameters):

    param = {}
    regex = r"^-(.+)" 
    
    # debug(sys_parameters)
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

    return param

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

    def function_negative_lit(self, value):
        return not value

    def __getitem__(self, lit: int) -> Any:
        i = abs(lit) 
        value = self.intepretation[i]
        if value is None:
            return None
        if lit < 0:
            value = self.function_negative_lit(value)
        return value
    
    def __setitem__(self, lit: int, value: Any):
        i = abs(lit)
        if lit < 0 :
            if not value is None:
                value = self.function_negative_lit(value)
            else:
                value = None
        self.intepretation[i] = value

class WeightFunction(SymmetricFunction):
    
    def __getitem__(self, lit: int) -> any:
        if lit is None:
            return 0
        return super().__getitem__(lit)

    def function_negative_lit(self, value):
        return value    


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
        #  it is a copy of ord_l, for now it is useless
        self.ord_l_origin : List[int] = ord_l.copy()


        # a function literals -> int that takes in input the literal l and gives as output the index of l in ord_lit:
        #   ord_i[l] = i 
        #   it is the inverse of ord_l function
        self.ord_i : dict[int, int] = ord_i

        # It is the index of the maximum (weight) undefined literal in ord_l 
        self.max_und : int = self.N - 1

        # It is the index of the minimum (weight) undefined literal in ord_l 
        self.min_und : int = 0 
        
        # all falses facts literals of the group
        self.falses_facts : List[int] = []


        # id of the group
        self.id = id
        self.id_autoinc = Group.autoincrement 
        Group.autoincrement += 1

    def increase_und(self):
        self.count_undef += 1
    
    def decrease_und(self):
        self.count_undef -= 1

    def add_false_lit(self, lit: int):
        self.falses_facts.append(lit)

    def remove_false_lit(self, lit: int):
        self.falses_facts.remove(lit)

    def set_max(self, l: int):
        self.max_und = self.ord_i[l]

    def set_min(self, l: int):
        self.min_und = self.ord_i[l]

    def update_max(self, I: SymmetricFunction, all = False):

        prev_max = self.ord_l[self.max_und] if not self.max_und is None and self.max_und < len(self.ord_l) else None
        
        if all:
            start = self.N - 1
        else:
            start = self.max_und - 1 if not self.max_und is None else -1
            # All are defined
            if start < 0:
                self.max_und = None
                return (None, prev_max)
            
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
        prev_min = self.ord_l[self.min_und] if not self.min_und is None else None
        
        if all:
            start = 0
        else:
            start = self.min_und + 1 if not self.min_und is None else self.N
            # All are defined
            if start >= self.N:
                self.min_und = None
                return (None, prev_min)

        for i in range(start, self.N, +1):
            l = self.ord_l[i]
            if I[l] is None:
                self.min_und = i
                new_min = self.ord_l[self.min_und]
                return (new_min, prev_min)
        
        # All are defined
        self.min_und = None
        return (None, prev_min)

    def update(self, I: SymmetricFunction, max, all = False):
        if max: 
            return self.update_max(I,all=all)
        else:
            return self.update_min(I,all=all)


    def print_group(self, atomNames):
        lit_names = ""
        for l in self.ord_l:
            lit_names += " " + get_name(atomNames,l) + " "
        debug(str(self), f"[{lit_names}]")

    def __str__(self) -> str:
        return str(self.id)

# removes useless literals
def simplifyLiterals(lits, aggregate: 'AggregateFunction', group: 'GroupFunction', max, I: SymmetricFunction ):
    '''
    Invariants: all literals inside lits have to be already assigned to a truth value
    '''
    
    G : Group = None
    
    for l in lits:
        if aggregate[l]:
            G = group[l]
            G.add_false_lit(not_(l))
        elif aggregate[not_(l)]:
            G = group[not_(l)]
            l = not_(l)
            G.add_false_lit(l)
        else:
            continue

    
        n = len(G.ord_l)
        li = G.ord_i[l]
        G.ord_i[l] = -1
        # updating position of next literals (shifting)
        for lit_i in range(li+1,n):
            lit = G.ord_l[lit_i]
            G.ord_i[lit] -= 1

        # removing literal
        G.ord_l.remove(l)
        G.N = len(G.ord_l)

        # updating max_und 
        G.update(I, max=max, all=True)

        # removing from aggregate
        aggregate[l] = False
        aggregate[not_(l)] = False

        assert G.count_undef >= 0

  
            

# This function returns the max UNDEFINED literal
def max_w(g: Group):
    max_und = g.max_und
    if(max_und is None):
        return None
    try:

        return g.ord_l[max_und]
    except Exception as e:
        print(f"g.ord_l {g.ord_l} max_und {max_und}")
        raise e

# This function returns the min UNDEFINED literal
def min_w(g: Group):
    min_und = g.min_und
    if(min_und is None):
        return None
    return g.ord_l[min_und]

def m_w(g: Group, max : bool):
    if max: 
        return max_w(g)
    else:
        return min_w(g)

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
    prefix = ""
    if lit is None:
        return "None"
    if lit < 0:
        prefix = "not "
    for a in atomNames:
        if atomNames[a] == abs(lit):
            return prefix + a
    debug(f"{lit} is not present in atomNames")
    assert False

def convert_array_to_string(name, array, atomNames, array_of_lits = True):

    res = ""
    res += f"{name} [ "
    for l in array:
        ln = get_name(atomNames=atomNames, lit = l) if array_of_lits else l
        res+= f"{ln} {l}, "
    res += "]"
    return res

def print_I(I, atomNames, aggregate, G = None, group = None, force_print = False, file=sys.stderr, none_atom = True, map_plit_slit : dict = None, specific_list_literals_slits = None, supplementaty_str = ""):
    if not DEBUG and not force_print:
        return
    assert (G is None and group is None) or (not G is None and not group is None) 
    if G is None:
        debug(f"Interpretation {supplementaty_str} ", end=" ", force_print=force_print, file=file)
    else:
        debug(f"Intepretation {supplementaty_str} for group: " + str(G), end=" ", force_print=force_print, file=file)
    for l in range(len(I.intepretation)):
        if I[l] is None and not none_atom:
            continue
        if (aggregate[l] or  aggregate[not_(l)]) and (G is None or group[l] == G) and \
            (specific_list_literals_slits is None or (map_plit_slit[l] in specific_list_literals_slits or map_plit_slit[-l] in specific_list_literals_slits)):
            debug(f"{get_name(atomNames,l)}[{l}]{f'[{map_plit_slit[l]}]' if map_plit_slit else ''}: {I[l]}",end=" ", force_print=force_print, file=file)
    debug("", force_print=force_print, file=file)

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
class Minimize(Enum):
    NO_MINIMIZATION = "default"
    MINIMAL = "min"
    CARDINALITY_MINIMAL = "cmin"

def is_true_in_reason(lit, group: GroupFunction):
    '''
    invariants: the literal is in the reason and it has been flipped (if true)
    '''
    g = group[lit]
    return g is None

def increment_f(l: int, current_subset_maximal, weight: WeightFunction, group: GroupFunction):

    g = group[l] 
    tr = False # true in reason
    if g is None:
        l = not_(l)
        g = group[l]
        tr = True # it means that it has been flipped, so it is true in reason (is the true_group[g])

    if len(g.ord_l) <= 1:
        return 0
    
    w = weight[l]
    if tr:  
        # TODO: FIX
        w_mw_g = 0
        w_mw_g = weight[g.ord_l[-1]] 
        return w_mw_g - w
    else:
        # i = g.ord_i[l]
        i = len(g.ord_l) - 1
        mw_g = weight[max_w(g)]
        current_l = g.ord_l[i]
        increment = w - mw_g
        while mw_g < weight[current_l]:
            if current_l in current_subset_maximal:
                increment = max(0,w - weight[current_l])
                # if it 0 means that in the current minimal subset is already present some literal of the same group
                #  but higher weight
                break
            i -= 1
            if i <= 0:
                break
            current_l = g.ord_l[i]
        return increment


def maximal_subset_sum_less_than_s_with_groups(literals: List[int], s: int, weight: WeightFunction,  group: GroupFunction):

    current_subset_maximal = []
    current_sum = 0

    for l in literals:
        inc = increment_f(l, current_subset_maximal, weight, group)
        if current_sum + inc <= s:
                # debug(f"{l} has been removed from reason because inc: {inc} is not enougth to arrive above {s} with current_sum {current_sum} ")
                current_sum += inc
                current_subset_maximal.append(l)
    
    return current_subset_maximal

def compute_increment_literals(literals: List[int], group: GroupFunction, weight: WeightFunction):
    increment = {}
    for l in literals:
        g : Group
        g = group[l] 
        tg = False
        if g is None:
            g = group[not_(l)]
            tg = True
            # l = not_(l)
        assert not g is None 
        mw_g = max_w(g)
        w_mw_g = weight[mw_g]
        
        w = weight[l]
        assert not w  is None
        i : int
        if tg:  
            w_mw_g = weight[g.ord_l[-1]]
            # print_err(f"w_mw_g {w_mw_g} l {l} w {w}" )
            # l is true: if it was false in the worst case the maxiumum literal of the group could be true
            i = w_mw_g - w
        else:
            # l is false: if it was true it would be the true_group[g] 
            i = w - w_mw_g
        increment[l] = i

    return increment

def get_all_lit_below_you(lit: int, group: GroupFunction, I: SymmetricFunction, reason: List[int]):

    lit_c = lit
    res = []
    res.append(lit_c)
    g = group[lit]
    if g is None:
        g = group[not_(lit)]
        lit = not_(lit)
        return set(res) # it is true so there are no literals below it

    start = g.ord_i[lit]
    for i in range(start-1,-1,-1):
        l = g.ord_l[i]
        if I[l] is None:
            # you found the maximum undefined
            break
        if l in reason:
            res.append(l)
    return set(res)


def maximum_subset_sum_less_than_s_with_groups(literals : List[int], s: int, group: GroupFunction, weight: dict, I: SymmetricFunction):
    
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

    # Fill the subset table in bottom up manner 
    for i in range (1, s + 1): 
        for j in range (1, n + 1):
            lit = literals[j - 1]
            w = weight[lit]
            subset[i][j] = subset[i][j - 1]
            if (i >= w) :
                sum_got = not subset[i][j] is None or not subset[i - w ][j - 1] is None
                if (sum_got):
                    correct_subset = True
                    k = j
                    while True:
                        if not subset[i - w][k - 1] is None:
                            # it is sufficient to controll that lit is not in subset[i - w][k - 1] and instead of also checking that 
                            # some literals of the same group are in the subset[i - w][k - 1] because since the literals are grouped and 
                            # ordered by weight if some literal before lit has been choose implies that also lit is already in subset[i - w][k - 1]
                            # and viceversa
                            if not lit in subset[i - w][k - 1]:
                                break
                        else:
                            correct_subset = False
                            break
                        k -= 1
                        if k <= 0:
                            break
                    with_lit_subset = subset[i - w][k - 1].union(get_all_lit_below_you(lit=lit, group=group, I=I, reason=literals)) if correct_subset else None
                    subset[i][j] = max( 
                            subset[i][j - 1], 
                            with_lit_subset,
                            key= lambda x: len(x) if not x is None else -1
                        )
                    
    maximum_subset = subset[s][n]

    # print(subset[:][n], file = sys.stderr)

    len_subset_max = len(maximum_subset) if subset[i][n] else -1
    for i in range(s+1):
        len_subset_max = len(maximum_subset) if not maximum_subset is None else -1
        len_subset = len(subset[i][n]) if not subset[i][n] is None  else -1
        # print("len_subset_max", len_subset_max)
        # print("len_subset", len_subset)
        if len_subset > len_subset_max:
            maximum_subset = subset[i][n]
        # print(f"mio dio come bestemmio {subset[i][n]}", file = sys.stderr)
    
    return list(maximum_subset)


#################################################################################################################################################
# END MINIMIZING REASON