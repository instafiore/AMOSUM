#include "amoclingo_algorithms.h"
#include "fiore/utils/logger.h"

// Binary search on the #amomaximize value.
// Invariants: lo is the value of the best model (minPossibleValue - 1 before any model), every bound <= lo is SAT;
// unsatBound is the smallest bound known UNSAT (maxPossibleValue + 1 before any UNSAT call). The open bounds are
// lo + 1 .. unsatBound - 1: each call tries the middle one, and the search ends when none is left.
fiore::clingo::ClingoResult binarySearch(AmosumClingoSolver& solver) noexcept{
    AmosumConstraint* amomaximize = amoclingoSearch::amomaximize(solver);
    if(amomaximize == nullptr) return amoclingoSearch::solveOnce(solver);

    amoclingoSearch::prepare(solver);
    // the bound moves both ways: the clauses learnt under a bound must not outlive its call
    amoclingoSearch::useVolatileClauses(solver);

    const int64_t maxValue = amoclingoSearch::maxPossibleValue(amomaximize);
    const int64_t minValue = std::min(amoclingoSearch::minPossibleValue(amomaximize), maxValue);
    int64_t lo = minValue - 1;
    int64_t unsatBound = maxValue + 1;
    AmosumClingoAssignment* best = nullptr;
    fiore::clingo::ClingoResult lastUnsat;                  // the result of the last UNSAT call

    while(true){
        if(lo + 1 >= unsatBound){
            // no open bound left: best is optimal, or the minimum value (which constrains nothing) was UNSAT; at
            // least one call was UNSAT in that case (lo + 1 = minValue <= maxValue < the initial unsatBound)
            if(best != nullptr) return amoclingoSearch::optimum(best);
            return lastUnsat;
        }
        // the middle of the open bounds lo + 1 .. unsatBound - 1 (unsatBound - lo >= 2, so mid is one of them)
        const int64_t mid = lo + (unsatBound - lo) / 2;
        #ifdef __debug__
            LOG(fiore::LogLevel::INFO, "Binary search: SAT up to ", lo, ", UNSAT from ", unsatBound, ", bound ", mid);
        #endif
        amomaximize->updateBound(mid);

        auto [answerset, result] = solver.solve();
        if(result.isSat()){
            best = amoclingoSearch::acceptModel(answerset, result, best);
            lo = best->amosumCost;                          // >= mid: every bound up to the model's value is SAT
        }
        else if(result.isUnsat()){ unsatBound = mid; lastUnsat = result; }
        else return best != nullptr ? best->result : result;   // interrupted: best (if any) stays "unknown"
    }
}
