"""UNSAT-SAT search on the #amomaximize value (downwards), with a progression p (LUS with p = 1); mirrors
core/cpp/search/unsat_sat.cpp.

Invariants: unsat_bound is the smallest bound known UNSAT (max_possible_value + 1 before any UNSAT call); best (value v),
if any, is the last model, every bound <= v is SAT. The optimum is v as soon as v + 1 == unsat_bound.
"""
from amosum.core.py.search.algorithms import (accept_model, interrupted, max_possible_value, min_possible_value,
                                              optimum, solve_once)
from amosum.core.py.search.progression import Progression


def unsat_sat(backend, progression: Progression):
    amomaximize = backend.amomaximize()
    if amomaximize is None:
        return solve_once(backend)
    # after a model the bound goes back above it: the clauses learnt under a bound must not outlive its call
    backend.use_volatile_clauses()
    max_value = max_possible_value(amomaximize)
    min_value = min(min_possible_value(amomaximize), max_value)
    unsat_bound, step, best = max_value + 1, 0, None
    bound = max_value
    backend.set_bound(bound)
    while True:
        found, result = backend.solve()
        if result.is_sat():
            best = accept_model(found, best)
            step = 0                            # back to the last UNSAT bound
        elif result.is_unsat():
            unsat_bound = bound
            if best is None and unsat_bound <= min_value:
                return None, result             # UNSAT where the #amomaximize constrains nothing: no answer set
        else:
            return interrupted(best, result)
        if best is not None and best.amosum_cost + 1 >= unsat_bound:
            return optimum(best)
        # unsat_bound - p(step), clamped to the smallest bound still open (above the last model's value, or the minimum
        # possible value before any model)
        lower = best.amosum_cost + 1 if best is not None else min_value
        decrement = progression.at(step)
        bound = unsat_bound - decrement if unsat_bound - lower >= decrement else lower
        step += 1
        backend.set_bound(bound)
