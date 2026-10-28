"""Searches on the #amomaximize value (mirrors core/cpp/search/algorithms.h): sat_unsat, unsat_sat, binary_search (one
file each) on any backend, and the helpers they share (amosumSearch in C++). Pure Python.

A backend (AmosumSearchBackend in C++; amoclingo/py/solver.py and amowasp/py/solver.py) has:
    amomaximize() -> the #amomaximize constraint or None
    solve() -> (AmosumClingoAssignment or None, ClingoResult)      one solve call with the current bound
    set_bound(b)                                                    the bound of the #amomaximize for the next calls
    use_volatile_clauses()                                          the bound may decrease: clauses must not outlive a call
Each search returns (the best answer set or None, the ClingoResult: optimum proven or not).
"""
from typing import Optional, Tuple

from fiore.clingo.utility_clingo import ClingoResult

from amosum.core.py.program import AmosumClingoAssignment, AmosumConstraint
from amosum.core.py.search.progression import Progression


def max_possible_value(constraint: AmosumConstraint) -> int:
    """at most one member per group, possibly none (the aux group, of weight bound = 0, is left out)"""
    return sum(max([0] + [m.weight for m in g.members]) for g in constraint.groups if not g.aux)


def min_possible_value(constraint: AmosumConstraint) -> int:
    return sum(min([0] + [m.weight for m in g.members]) for g in constraint.groups if not g.aux)


def accept_model(found: AmosumClingoAssignment, best: Optional[AmosumClingoAssignment]) -> AmosumClingoAssignment:
    """a model of backend.solve(): the new best answer set, printed as not yet optimal"""
    found.result = ClingoResult.satisfiable().set_unknown_optimum()
    found.print_assignment()
    return found


def optimum(best: AmosumClingoAssignment) -> Tuple[AmosumClingoAssignment, ClingoResult]:
    """best is proven optimal"""
    return best, ClingoResult.satisfiable().set_optimum()


def interrupted(best: Optional[AmosumClingoAssignment], result: ClingoResult):
    return (best, ClingoResult.satisfiable().set_unknown_optimum()) if best is not None else (None, result)


def solve_once(backend):
    """without #amomaximize: one solve call"""
    return backend.solve()
