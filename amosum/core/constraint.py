"""The amosum constraints as data (pure Python: the WASP plugin reads them from JSON, without clingo).

A constraint has groups of members; a member is an atom (by name), positive or negated, with its weight. The clingo
side (amosum.core.program) builds them from the ground program; each solver resolves the names to its own literals.
"""
from typing import List

AMOMAXIMIZE = "amomaximize"


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

    def max_possible_value(self) -> int:
        """at most one member per group, possibly none (the aux group is left out)"""
        return sum(max([0] + [m.weight for m in g.members]) for g in self.groups if not g.aux)

    def min_possible_value(self) -> int:
        return sum(min([0] + [m.weight for m in g.members]) for g in self.groups if not g.aux)

    def to_json(self):
        return {"id": self.id, "type": self.type, "ge": self.ge, "bound": self.bound,
                "groups": [g.to_json() for g in self.groups], "group_atoms": self.group_atoms}

    @staticmethod
    def from_json(j):
        return AmosumConstraint(j["id"], j["type"], j["ge"], j["bound"], [ProgramGroup.from_json(g) for g in j["groups"]],
                                j["group_atoms"])


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
