"""Binary search on the #amomaximize value; mirrors core/cpp/search/binary.cpp.

Invariants: lo is the value of the best model (min_possible_value - 1 before any model), every bound <= lo is SAT;
unsat_bound is the smallest bound known UNSAT (max_possible_value + 1 before any UNSAT call). The open bounds are
lo + 1 .. unsat_bound - 1: each call tries the middle one, and the search ends when none is left.
"""
from amosum.core.py.search.algorithms import (accept_model, interrupted, max_possible_value, min_possible_value,
                                              optimum, solve_once)


def binary_search(backend):
    amomaximize = backend.amomaximize()
    if amomaximize is None:
        return solve_once(backend)
    # the bound moves both ways: the clauses learnt under a bound must not outlive its call
    backend.use_volatile_clauses()
    max_value = max_possible_value(amomaximize)
    min_value = min(min_possible_value(amomaximize), max_value)
    lo, unsat_bound, best, last_unsat = min_value - 1, max_value + 1, None, None
    while True:
        if lo + 1 >= unsat_bound:
            # no open bound left: best is optimal, or the minimum value (which constrains nothing) was UNSAT
            return optimum(best) if best is not None else (None, last_unsat)
        mid = lo + (unsat_bound - lo) // 2      # unsat_bound - lo >= 2: mid is one of the open bounds
        backend.set_bound(mid)
        found, result = backend.solve()
        if result.is_sat():
            best = accept_model(found, best)
            lo = best.amosum_cost               # >= mid: every bound up to the model's value is SAT
        elif result.is_unsat():
            unsat_bound, last_unsat = mid, result
        else:
            return interrupted(best, result)
