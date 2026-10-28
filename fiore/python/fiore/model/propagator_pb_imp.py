"""What a PropagatorPBConstraint runs in each solver thread (mirrors PropagatorPBConstraintImplementation of
fiore/cpp/fiore/model/propagatorPBImp.h; pure Python: the WASP plugin uses it too). It works on solver literals:

    on_literals_true(changes, dl, force) -> the derived literals (BOTTOM: a conflict); force: first propagation at level 0
    on_literals_undefined(changes)
    get_reason_for_literal(derived) -> the literals, false in the current assignment, that make the derivation valid
    init_propagation() -> the derived literals before search, at every solve call (BOTTOM: the bound is violated already)

A derivation d is the clause [d] + get_reason_for_literal(d) ([] + the reason for BOTTOM).
"""
from typing import List, Sequence


class PropagatorPBConstraintImplementation:

    def on_literals_true(self, changes: Sequence[int], dl: int, force: bool = False) -> List[int]:
        raise NotImplementedError

    def on_literals_undefined(self, changes: Sequence[int]) -> None:
        raise NotImplementedError

    def get_reason_for_literal(self, literal: int) -> List[int]:
        raise NotImplementedError

    def init_propagation(self) -> List[int]:
        raise NotImplementedError
