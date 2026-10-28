#include "amosum/core/cpp/search/algorithms.h"
#include "fiore/utils/logger.h"

// SAT-UNSAT search on the #amomaximize value (upwards), with a progression p (LSU with p = 1).
// Invariants: best (value v) is the last model, every bound <= v is SAT; unsatBound is the smallest bound known UNSAT
// (maxPossibleValue + 1 before any UNSAT call). The optimum is v as soon as v + 1 == unsatBound.
fiore::clingo::ClingoResult satUnsat(AmosumSearchBackend& backend, const Progression& progression) noexcept{
    AmosumConstraint* amomaximize = backend.amomaximize();
    if(amomaximize == nullptr) return amosumSearch::solveOnce(backend);

    // with p = 1 the bound only increases: a clause learnt under a bound holds for every later one; otherwise the bound
    // goes back after an UNSAT call, and the clauses learnt under a bound must not outlive its call
    if(!progression.isLinear()) backend.useVolatileClauses();

    int64_t unsatBound = amosumSearch::maxPossibleValue(amomaximize) + 1;
    size_t step = 0;
    AmosumClingoAssignment* best = nullptr;

    // the first call with the smallest value of the #amomaximize: it constrains nothing (negative weights included)
    backend.setBound(amosumSearch::minPossibleValue(amomaximize));
    while(true){
        auto [answerset, result] = backend.solve();

        if(result.isSat()){
            best = amosumSearch::acceptModel(answerset, result, best);
        }
        else if(result.isUnsat()){
            if(best == nullptr) return result;              // UNSAT at the minimum value: no answer set
            unsatBound = amomaximize->bound;                // every bound in (v, unsatBound) is still open
            step = 0;                                       // back to the last model's value
        }
        else return best != nullptr ? best->result : result;   // interrupted: best (if any) stays "unknown"

        const int64_t value = best->amosumCost;
        if(value + 1 >= unsatBound) return amosumSearch::optimum(best);

        // v + p(step), clamped to the largest bound still open (below unsatBound, so never above the maximum value)
        const int64_t increment = progression.at(step);
        const int64_t next = (unsatBound - 1 - value >= increment) ? value + increment : unsatBound - 1;
        ++step;
        #ifdef __debug__
            LOG(fiore::LogLevel::INFO, "SAT-UNSAT (", progression.expression, "): value ", value, ", UNSAT from ", unsatBound, ", next bound ", next);
        #endif
        backend.setBound(next);
    }
}
