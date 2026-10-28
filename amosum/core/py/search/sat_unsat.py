"""SAT-UNSAT search on the #amomaximize value (upwards), with a progression p (LSU with p = 1); mirrors
core/cpp/search/sat_unsat.cpp.

Invariants: best (value v) is the last model, every bound <= v is SAT; unsat_bound is the smallest bound known UNSAT
(max_possible_value + 1 before any UNSAT call). The optimum is v as soon as v + 1 == unsat_bound.
"""
from amosum.core.py.search.algorithms import (accept_model, interrupted, max_possible_value, min_possible_value,
                                              optimum, solve_once)
from amosum.core.py.search.progression import Progression


def sat_unsat(backend, progression: Progression):
    amomaximize = backend.amomaximize()
    if amomaximize is None:
        return solve_once(backend)
    # with p = 1 the bound only increases: a clause learnt under a bound holds for every later one
    if not progression.is_linear():
        backend.use_volatile_clauses()
    unsat_bound = max_possible_value(amomaximize) + 1
    step, best = 0, None
    # the first call with the smallest value of the #amomaximize: it constrains nothing (negative weights included)
    bound = min_possible_value(amomaximize)
    backend.set_bound(bound)
    while True:
        found, result = backend.solve()
        if result.is_sat():
            best = accept_model(found, best)
        elif result.is_unsat():
            if best is None:
                return None, result             # UNSAT at the minimum value: no answer set
            unsat_bound, step = bound, 0        # back to the last model's value
        else:
            return interrupted(best, result)
        if best.amosum_cost + 1 >= unsat_bound:
            return optimum(best)
        # v + p(step), clamped to the largest bound still open (below unsat_bound, so never above the maximum value)
        increment = progression.at(step)
        bound = best.amosum_cost + increment if unsat_bound - 1 - best.amosum_cost >= increment else unsat_bound - 1
        step += 1
        backend.set_bound(bound)
