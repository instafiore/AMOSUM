"""WASP's plugin protocol on a PropagatorPBConstraintImplementation (no C++ counterpart: WASP runs Python plugins; pure
Python, it runs inside WASP's interpreter).

WASP calls the functions of a plugin module (--plugins-file=<module>):
    getLiterals(n, *level0)       n variables, the literals fixed at level 0; returns the literals to watch
    simplifyAtLevelZero()         returns unit clauses (a literal and its negation: inconsistent)
    onLiteralTrue(lit, dl)        a watched literal is true; returns the derived literals (a false one: a conflict)
    getReasonForLiteral(lit)      the literals, all false, such that lit or them is a clause
    onLiteralsUndefined(dl, *lits)
and fills the module attribute atomNames (atom name -> variable). `install` binds these functions to a WaspPlugin.
"""
from typing import Callable, Dict, List

from fiore.utils.constants import BOTTOM


class WaspPlugin:
    """`build(atom_names, level0)` (level0: the literals fixed at level 0) returns (implementation, watched literals,
    unit literals at level 0); the implementation is a PropagatorPBConstraintImplementation (fiore.model.propagator_pb_imp)."""

    def __init__(self, atom_names: Dict[str, int], build: Callable):
        self.atom_names = atom_names
        self.build = build
        self.implementation = None
        self.units: List[int] = []
        self.level0: List[int] = []
        self.reasons: Dict[int, List[int]] = {}
        self.any_variable = 1

    def get_literals(self, *lits) -> List[int]:
        self.level0 = list(lits[1:])
        self.implementation, watched, self.units = self.build(self.atom_names, set(self.level0))
        if watched:
            self.any_variable = abs(watched[0])
        return watched

    def simplify_at_level_zero(self) -> List[int]:
        units = list(self.units)
        derived = self.implementation.init_propagation() + self.implementation.on_literals_true([], 0, force=True)
        if BOTTOM in derived:
            return [self.any_variable, -self.any_variable]          # inconsistent at level 0
        return units + derived

    def on_literal_true(self, lit: int, dl: int) -> List[int]:
        derived = self.implementation.on_literals_true([lit], dl)
        out = []
        for d in derived:
            reason = list(self.implementation.get_reason_for_literal(d))
            if d == BOTTOM:
                # a conflict: WASP wants a false literal and its reason, with a literal of the current level in the
                # reason; not(lit) is false (lit was just set true) and of this level: the conflict literal, and in the
                # reason too (with lazy propagation the other literals may all be of lower levels), as the old version
                d = -lit
                if d not in reason:
                    reason.append(d)
            elif dl > 0 and -lit not in reason and d != -lit:
                # WASP wants a literal of the current decision level in every reason (clingo does not): not(lit) is
                # false and of this level; adding a false literal keeps the clause valid (only weaker)
                reason.append(-lit)
            self.reasons[d] = reason
            out.append(d)
        return out

    def get_reason_for_literal(self, lit: int) -> List[int]:
        return self.reasons.get(lit, [])

    def on_literals_undefined(self, *lits) -> None:
        self.implementation.on_literals_undefined(lits[1:])


def install(module_globals: dict, plugin: WaspPlugin) -> None:
    """Binds WASP's plugin functions in a module to `plugin`."""
    module_globals["getLiterals"] = plugin.get_literals
    module_globals["simplifyAtLevelZero"] = plugin.simplify_at_level_zero
    module_globals["onLiteralTrue"] = plugin.on_literal_true
    module_globals["getReasonForLiteral"] = plugin.get_reason_for_literal
    module_globals["onLiteralsUndefined"] = plugin.on_literals_undefined
