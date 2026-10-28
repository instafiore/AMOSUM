"""The amosum aggregates of a ground program (mirrors core/cpp/program.h and program.cpp).

    Member, ProgramGroup, AmosumConstraint     an aggregate: groups of members (atoms by name, positive or negated, with
                                               their weights); the clingo and the WASP sides resolve the names
    read_aggregates                            AmosumProgram::AmosumGrounder::ground: the constraints of the ground program,
                                               normalized (set semantics), and the ground rules that define the OR atoms
    AmosumClingoAssignment                     a model with its #amomaximize value (amosum_cost)

The rewritten program describes each aggregate with atoms:
    __group__(L, "+"|"-", W, G, ID)    element L (or not L for "-") of weight W in group G of aggregate ID
    __lb__(B, ID) / __ub__(B, ID)      >= B / <= B
    __aux__(ID, TYPE)                  ge_amo, ge_eo, le_amo, le_eo, amomaximize
A tuple in several groups counts once, through a __tuple__ OR atom; equal weights in a group are one __same__ OR atom.

Pure Python except read_aggregates (clingo, imported there): the WASP plugin uses the data classes.
"""
import re
from typing import Dict, List, Optional, Tuple

from fiore.model.assignment import ClingoAssignment

from amosum.core.py.settings import (AMOMAXIMIZE, AUX_REGEX, PREDICATE_AUX, PREDICATE_ELEMENT, PREDICATE_GROUP,
                                     PREDICATE_LB, PREDICATE_SAME, PREDICATE_TUPLE, PREDICATE_UB)


class Member:
    """An element of a group: an atom (by name), positive or negated, with its weight."""
    __slots__ = ("name", "positive", "weight")

    def __init__(self, name: str, positive: bool, weight: int):
        self.name, self.positive, self.weight = name, positive, weight

    def literal_text(self) -> str:
        return self.name if self.positive else f"not {self.name}"

    def to_json(self): return [self.name, self.positive, self.weight]

    @staticmethod
    def from_json(j): return Member(j[0], j[1], j[2])


class ProgramGroup:
    def __init__(self, id: str, members: List[Member], aux: bool = False):
        self.id, self.members, self.aux = id, members, aux

    def to_json(self): return {"id": self.id, "members": [m.to_json() for m in self.members], "aux": self.aux}

    @staticmethod
    def from_json(j): return ProgramGroup(j["id"], [Member.from_json(m) for m in j["members"]], j["aux"])


class AmosumConstraint:
    def __init__(self, id: str, type: str, ge: bool, bound: int, groups: List[ProgramGroup], group_atoms: List[str]):
        self.id, self.type, self.ge, self.bound, self.groups, self.group_atoms = id, type, ge, bound, groups, group_atoms

    @property
    def eo(self) -> bool: return self.type in ("ge_eo", "le_eo")

    @property
    def is_amomaximize(self) -> bool: return self.type == AMOMAXIMIZE

    def to_json(self):
        return {"id": self.id, "type": self.type, "ge": self.ge, "bound": self.bound,
                "groups": [g.to_json() for g in self.groups], "group_atoms": self.group_atoms}

    @staticmethod
    def from_json(j):
        return AmosumConstraint(j["id"], j["type"], j["ge"], j["bound"], [ProgramGroup.from_json(g) for g in j["groups"]],
                                j["group_atoms"])


class _Element:
    __slots__ = ("member", "group", "tuple", "aux")

    def __init__(self, member: Member, group: str, tuple: str, aux: bool):
        self.member, self.group, self.tuple, self.aux = member, group, tuple, aux


class _Aggregate:
    def __init__(self):
        self.elements: List[_Element] = []
        self.group_atoms: List[str] = []
        self.bound = 0
        self.ge = True
        self.type = ""


def _normalize(aggregate_id: str, eo: bool, elements: List[_Element], rules: List[str]) -> List[ProgramGroup]:
    """program level normalization (as normalize in amosum_program.cpp); the OR atoms are defined in `rules`"""
    by_id: Dict[str, ProgramGroup] = {}
    by_tuple: Dict[str, List[_Element]] = {}
    for e in elements:
        g = by_id.setdefault(e.group, ProgramGroup(e.group, []))
        g.aux = g.aux or e.aux
        g.members.append(Member(e.member.name, e.member.positive, e.member.weight))
        if e.tuple:
            by_tuple.setdefault(e.tuple, []).append(e)

    def disjunction(name: str, members: List[Member]) -> Member:
        for m in members:
            rules.append(f"{name} :- {m.literal_text()}.")
        return Member(name, True, 0)

    # a tuple in several groups: x = OR of its element atoms, weight w in a group of its own; the element atoms stay in
    # their groups with weight 0; EO: not x in x's group too, so that it is exactly-one
    extra: List[ProgramGroup] = []
    for t in sorted(by_tuple):
        es = by_tuple[t]
        if len(es) < 2:
            continue
        for e in es:
            g = by_id[e.group]
            for m in g.members:
                if m.name == e.member.name and m.positive == e.member.positive:
                    m.weight = 0
                    break
        x = disjunction(f"{PREDICATE_TUPLE}({aggregate_id},{t})", [e.member for e in es])
        x.weight = es[0].member.weight
        group = ProgramGroup(PREDICATE_TUPLE + t, [x])
        if eo:
            group.members.append(Member(x.name, False, 0))
        extra.append(group)

    # equal weights in a group: y = OR of those members, with that weight
    for gid in sorted(by_id):
        g = by_id[gid]
        by_weight: Dict[int, List[Member]] = {}
        for m in g.members:
            by_weight.setdefault(m.weight, []).append(m)
        members = []
        for w in sorted(by_weight):
            ms = by_weight[w]
            if len(ms) == 1:
                members.append(ms[0])
            else:
                y = disjunction(f"{PREDICATE_SAME}({aggregate_id},{gid},{w})", ms)
                y.weight = w
                members.append(y)
        g.members = members
    return [by_id[gid] for gid in sorted(by_id)] + extra


def read_aggregates(program, show_members: bool = False) -> Tuple[List[AmosumConstraint], str]:
    """The constraints of the ground program (a fiore.model.program.Program) and the ground rules to add (the OR
    atoms, the __group__ atoms false; with show_members, #show for the atoms the propagator needs, for a program that
    shows only some atoms)."""
    import clingo
    aggregates: Dict[str, _Aggregate] = {}
    for literal in program.atoms():
        s = literal.symbol
        if s is None or s.type != clingo.SymbolType.Function:
            continue
        args = s.arguments
        if s.name == PREDICATE_GROUP and len(args) == 5:
            a = aggregates.setdefault(str(args[4]), _Aggregate())
            name = str(args[0])
            if program.literal(name) is None:
                continue
            plus = str(args[1]) == '"+"'
            tuple = ""
            if args[0].type == clingo.SymbolType.Function and args[0].name == PREDICATE_ELEMENT and len(args[0].arguments) == 3:
                tuple = str(args[0].arguments[1])
            a.elements.append(_Element(Member(name, plus, args[2].number), str(args[3]), tuple, not plus))
            a.group_atoms.append(literal.name)
        elif s.name in (PREDICATE_LB, PREDICATE_UB) and len(args) == 2:
            a = aggregates.setdefault(str(args[1]), _Aggregate())
            a.bound = args[0].number
            a.ge = s.name == PREDICATE_LB
        elif s.name == PREDICATE_AUX and len(args) == 2:
            aggregates.setdefault(str(args[0]), _Aggregate()).type = str(args[1])

    rules: List[str] = []
    constraints = []
    for aggregate_id in sorted(aggregates):
        a = aggregates[aggregate_id]
        if not a.elements:
            continue
        eo = a.type in ("ge_eo", "le_eo")
        groups = _normalize(aggregate_id, eo, a.elements, rules)
        constraints.append(AmosumConstraint(aggregate_id, a.type, a.ge, a.bound, groups, a.group_atoms))
        # the __group__ atoms only describe the aggregate: false (as the C++ propagator does at init)
        rules += [f":- {g}." for g in a.group_atoms]
    if show_members and constraints:
        rules += [f"#show {PREDICATE_ELEMENT}/3.", f"#show {PREDICATE_TUPLE}/2.", f"#show {PREDICATE_SAME}/3.",
                  f"#show {PREDICATE_AUX}/2."]
    if sum(1 for c in constraints if c.is_amomaximize) > 1:
        raise ValueError("At most one amomaximize statement can be declared!")
    return constraints, "\n".join(rules) + "\n"


def model_value(constraint: AmosumConstraint, is_true) -> int:
    """the value of a model for the constraint (at most one true member per group: the first one counts);
    is_true(atom name) says whether an atom is true in the model"""
    value = 0
    for g in constraint.groups:
        for m in g.members:
            if is_true(m.name) == m.positive:
                value += m.weight
                break
    return value


class AmosumClingoAssignment(ClingoAssignment):
    """a model with the value of the #amomaximize (amosum_cost, 0 without one); the auxiliary atoms are not printed"""

    def __init__(self, shown: List[str], amomaximize: Optional[AmosumConstraint] = None, is_true=None, solve_time: float = 0.0):
        super().__init__(shown, solve_time, AUX_REGEX)
        self.has_amomaximize = amomaximize is not None
        self.amosum_cost = model_value(amomaximize, is_true) if amomaximize is not None else 0

    def print_assignment(self, prefix: str = "") -> None:
        if self.has_amomaximize:
            prefix += f"Amomaximize Cost: {self.amosum_cost} "
        super().print_assignment(prefix)
