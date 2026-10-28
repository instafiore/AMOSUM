#include "amosum/core/cpp/search/algorithms.h"
#include "fiore/utils/logger.h"

// Binary search on the #amomaximize value.
// Invariants: lo is the value of the best model (minPossibleValue - 1 before any model), every bound <= lo is SAT;
// unsatBound is the smallest bound known UNSAT (maxPossibleValue + 1 before any UNSAT call). The open bounds are
// lo + 1 .. unsatBound - 1: each call tries the middle one, and the search ends when none is left.
fiore::clingo::ClingoResult binarySearch(AmosumSearchBackend& backend) noexcept{
    AmosumConstraint* amomaximize = backend.amomaximize();
    if(amomaximize == nullptr) return amosumSearch::solveOnce(backend);

    // the bound moves both ways: the clauses learnt under a bound must not outlive its call
    backend.useVolatileClauses();

    const int64_t maxValue = amosumSearch::maxPossibleValue(amomaximize);
    const int64_t minValue = std::min(amosumSearch::minPossibleValue(amomaximize), maxValue);
    int64_t lo = minValue - 1;
    int64_t unsatBound = maxValue + 1;
    AmosumClingoAssignment* best = nullptr;
    fiore::clingo::ClingoResult lastUnsat;                  // the result of the last UNSAT call

    while(true){
        if(lo + 1 >= unsatBound){
            // no open bound left: best is optimal, or the minimum value (which constrains nothing) was UNSAT; at
            // least one call was UNSAT in that case (lo + 1 = minValue <= maxValue < the initial unsatBound)
            if(best != nullptr) return amosumSearch::optimum(best);
            return lastUnsat;
        }
        // the middle of the open bounds lo + 1 .. unsatBound - 1 (unsatBound - lo >= 2, so mid is one of them)
        const int64_t mid = lo + (unsatBound - lo) / 2;
        #ifdef __debug__
            LOG(fiore::LogLevel::INFO, "Binary search: SAT up to ", lo, ", UNSAT from ", unsatBound, ", bound ", mid);
        #endif
        backend.setBound(mid);

        auto [answerset, result] = backend.solve();
        if(result.isSat()){
            best = amosumSearch::acceptModel(answerset, result, best);
            lo = best->amosumCost;                          // >= mid: every bound up to the model's value is SAT
        }
        else if(result.isUnsat()){ unsatBound = mid; lastUnsat = result; }
        else return best != nullptr ? best->result : result;   // interrupted: best (if any) stays "unknown"
    }
}
