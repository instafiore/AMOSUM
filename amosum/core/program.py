"""The amosum aggregates of a ground program (port of AmosumGrounder::ground and normalize in amosum_program.cpp).

The rewritten program describes each aggregate with atoms:
    __group__(L, "+"|"-", W, G, ID)    element L (or not L for "-") of weight W in group G of aggregate ID
    __lb__(B, ID) / __ub__(B, ID)      >= B / <= B
    __aux__(ID, TYPE)                  ge_amo, ge_eo, le_amo, le_eo, amomaximize

`read_aggregates` builds the constraints with set semantics (a tuple in several groups counts once, through a
__tuple__ OR atom; equal weights in a group are one __same__ OR atom) and the ground rules that define those atoms.
Members are atom names: the clingo and the WASP sides resolve them to their own literals.
"""
from typing import Dict, List, Optional, Tuple

import clingo

from fiore.program import Program

from amosum.core.constraint import AMOMAXIMIZE, AmosumConstraint, Member, ProgramGroup, model_value

PREDICATE_GROUP, PREDICATE_LB, PREDICATE_UB, PREDICATE_AUX = "__group__", "__lb__", "__ub__", "__aux__"
PREDICATE_ELEMENT, PREDICATE_TUPLE, PREDICATE_SAME = "__element__", "__tuple__", "__same__"


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


def read_aggregates(program: Program, show_members: bool = False) -> Tuple[List[AmosumConstraint], str]:
    """The constraints of the ground program and the ground rules to add (the OR atoms, the __group__ atoms false;
    with show_members, #show for the atoms the propagator needs, for a program that shows only some atoms)."""
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
