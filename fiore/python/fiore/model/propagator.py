"""The base of the clingo propagators (mirrors ClingoPropagator of fiore/cpp/fiore/model/propagator.h): a name, the type
of the clauses added during search (learnt or volatile) and how to add a clause."""
from typing import Sequence, Union

import clingo


class ClingoPropagator:

    def __init__(self, name: str = "", volatile: bool = False):
        self.name = name
        self.volatile = volatile        # clingo_clause_type_volatile: the clauses are deleted after the solve call

    def update_clause_type(self, volatile: bool) -> None:
        """volatile clauses are deleted after the current solve call: for a bound that can also decrease between calls"""
        self.volatile = volatile

    def is_volatile_clause_type(self) -> bool:
        return self.volatile

    def add_clause(self, control: Union[clingo.PropagateInit, clingo.PropagateControl], clause: Sequence[int],
                   volatile: bool = None) -> bool:
        """True when the clause is a conflict (the caller must return immediately). At init a clause is permanent;
        during search it has the given type (None: the propagator's clause type) and is propagated at once."""
        if isinstance(control, clingo.PropagateInit):
            return not control.add_clause(clause)
        tag = self.volatile if volatile is None else volatile
        return not control.add_clause(clause, tag=tag) or not control.propagate()
