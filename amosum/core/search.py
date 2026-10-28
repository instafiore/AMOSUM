"""The searches on the #amomaximize value (port of amoclingo_{sat_unsat,unsat_sat,binary}.cpp), on any backend:

    backend.solve() -> (AnswerSet | None, Result)     one solve call with the current bound
    backend.set_bound(b)                               the bound of the #amomaximize for the next calls
    backend.use_volatile_clauses()                     the bound may decrease: clauses must not outlive a call

Each search returns (the best answer set or None, the Result: optimum proven or not).
"""
from typing import Optional, Tuple

from fiore.result import Result

from amosum.core.output import AnswerSet
from amosum.core.progression import Progression
from amosum.core.constraint import AmosumConstraint

SAT_UNSAT, UNSAT_SAT, BINARY = "sat-unsat", "unsat-sat", "binary"
STRATEGIES = [SAT_UNSAT, UNSAT_SAT, BINARY]


def _accept(found: AnswerSet, best: Optional[AnswerSet]) -> AnswerSet:
    """a model of the search: the new best, printed as not yet optimal"""
    found.result = Result.satisfiable().set_unknown_optimum()
    found.print_assignment()
    return found


def _optimum(best: AnswerSet) -> Tuple[AnswerSet, Result]:
    return best, Result.satisfiable().set_optimum()


def _interrupted(best: Optional[AnswerSet], result: Result) -> Tuple[Optional[AnswerSet], Result]:
    return (best, Result.satisfiable().set_unknown_optimum()) if best is not None else (None, result)


def solve_once(backend) -> Tuple[Optional[AnswerSet], Result]:
    """without #amomaximize: one solve call"""
    return backend.solve()


def sat_unsat(backend, constraint: AmosumConstraint, progression: Progression):
    """upwards from the minimum possible value: after a model of value v the bound is v + p(step), back to v + 1 with
    step 0 after UNSAT; never at a bound known UNSAT nor above the maximum possible value"""
    if not progression.is_linear():
        backend.use_volatile_clauses()          # with p = 1 the bound only increases: clauses can stay learnt
    unsat_bound = constraint.max_possible_value() + 1
    step, best = 0, None
    bound = constraint.min_possible_value()
    backend.set_bound(bound)
    while True:
        found, result = backend.solve()
        if result.is_sat():
            best = _accept(found, best)
        elif result.is_unsat():
            if best is None:
                return None, result             # UNSAT at the minimum value: no answer set
            unsat_bound, step = bound, 0
        else:
            return _interrupted(best, result)
        if best.value + 1 >= unsat_bound:
            return _optimum(best)
        increment = progression.at(step)
        bound = best.value + increment if unsat_bound - 1 - best.value >= increment else unsat_bound - 1
        step += 1
        backend.set_bound(bound)


def unsat_sat(backend, constraint: AmosumConstraint, progression: Progression):
    """downwards from the maximum possible value: after an UNSAT call at bound u the bound is u - p(step), back to u - 1
    with step 0 after a model; never at a value known SAT nor below the minimum possible value"""
    backend.use_volatile_clauses()
    max_value = constraint.max_possible_value()
    min_value = min(constraint.min_possible_value(), max_value)
    unsat_bound, step, best = max_value + 1, 0, None
    bound = max_value
    backend.set_bound(bound)
    while True:
        found, result = backend.solve()
        if result.is_sat():
            best = _accept(found, best)
            step = 0
        elif result.is_unsat():
            unsat_bound = bound
            if best is None and unsat_bound <= min_value:
                return None, result             # UNSAT where the #amomaximize constrains nothing
        else:
            return _interrupted(best, result)
        if best is not None and best.value + 1 >= unsat_bound:
            return _optimum(best)
        lower = best.value + 1 if best is not None else min_value
        decrement = progression.at(step)
        bound = unsat_bound - decrement if unsat_bound - lower >= decrement else lower
        step += 1
        backend.set_bound(bound)


def binary(backend, constraint: AmosumConstraint, progression: Progression = None):
    """between the best model's value (every bound up to it is SAT) and the smallest bound known UNSAT, each call
    tries the middle bound"""
    backend.use_volatile_clauses()
    max_value = constraint.max_possible_value()
    min_value = min(constraint.min_possible_value(), max_value)
    lo, unsat_bound, best, last_unsat = min_value - 1, max_value + 1, None, None
    while True:
        if lo + 1 >= unsat_bound:
            return _optimum(best) if best is not None else (None, last_unsat)
        mid = lo + (unsat_bound - lo) // 2
        backend.set_bound(mid)
        found, result = backend.solve()
        if result.is_sat():
            best = _accept(found, best)
            lo = best.value
        elif result.is_unsat():
            unsat_bound, last_unsat = mid, result
        else:
            return _interrupted(best, result)


def run(strategy: str, backend, constraint: Optional[AmosumConstraint], progression: Progression):
    if constraint is None:
        return solve_once(backend)
    return {SAT_UNSAT: sat_unsat, UNSAT_SAT: unsat_sat, BINARY: binary}[strategy](backend, constraint, progression)
