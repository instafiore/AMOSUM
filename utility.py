# utility module

from typing import List

FOCUSED_GROUP = 2
FOCUSING = False

DEBUG = False
def debug(*message: str, G: 'Group' = None , end ="\n"):
    if DEBUG and ( G is None or G.id == FOCUSED_GROUP or not FOCUSING):
        print(message, end=end)

class Interpretation:
    
    def __init__(self, N) -> None:
        # interpretation
        self.intepretation : List[bool] = [None] * N

    def __getitem__(self, lit: int) -> bool:
        i = abs(lit) 
        value = self.intepretation[i]
        if lit < 0:
            value = not value
        return value
    
    def __setitem__(self, lit: int, value: bool):
        i = abs(lit)
        if lit < 0 :
            if not value is None:
                value = not value
            else:
                value = None
        self.intepretation[i] = value


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

    def update_max(self, I: Interpretation, all = False):
        start = self.max_und - 1
        prev_max = self.ord_l[self.max_und]
        
        # All are defined
        if start < 0:
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
        return (None, prev_max)
    
    def update_min(self, I: Interpretation, all = False):
        start = self.min_und + 1
        prev_min = self.ord_l[self.min_und]
        
        # All are defined
        if start >= self.N:
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

class PerfectHash:

    def __getitem__(self, lit: int) -> any:
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
    pass

class WeightFunction(PerfectHash):
    
    def __getitem__(self, lit: int) -> any:
        if lit is None:
             raise ValueError("Invalid argument: 'lit' must be an integer.")
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
    if lit < 0:
        prefix = "not "
    for a in atomNames:
        if atomNames[a] == abs(lit):
            return prefix + a


def print_I(I, atomNames, aggregate, G = None, group = None):
    if not DEBUG:
        return
    assert (G is None and group is None) or (not G is None and not group is None) 
    if G is None:
        debug("Interpretation")
    else:
        debug("Intepretation for group: " + str(G))
    for l in range(len(I.intepretation)):
        if (aggregate[l] or  aggregate[not_(l)]) and (G is None or group[l] == G):
            debug(get_name(atomNames,l), I[l])

def print_perfect_hash(ph: PerfectHash, atomNames, aggregate):
    if not DEBUG:
        return
    
    for i in range(len(ph.values)):
        l = i
        if l >= ph.N:
            l = not_(l - ph.N)
        if aggregate[l] or aggregate[not_(l)]:
            debug(get_name(atomNames,l), str(ph[l]))

def print_weights(weight: WeightFunction, atomNames, aggregate):
    debug("Weights")
    print_perfect_hash(weight, atomNames, aggregate)

def print_groups(group: GroupFunction, atomNames, aggregate):
    debug("Groups")
    print_perfect_hash(group, atomNames, aggregate)