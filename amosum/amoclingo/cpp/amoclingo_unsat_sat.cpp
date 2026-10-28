#include "amoclingo_algorithms.h"
#include "fiore/utils/logger.h"

// UNSAT-SAT search on the #amomaximize value (downwards), with a progression p (LUS with p = 1; the mirror of satUnsat).
// Invariants: unsatBound is the smallest bound known UNSAT (maxPossibleValue + 1 before any UNSAT call); best (value v),
// if any, is the last model, every bound <= v is SAT. The optimum is v as soon as v + 1 == unsatBound.
fiore::clingo::ClingoResult unsatSat(AmosumClingoSolver& solver, const Progression& progression) noexcept{
    AmosumConstraint* amomaximize = amoclingoSearch::amomaximize(solver);
    if(amomaximize == nullptr) return amoclingoSearch::solveOnce(solver);

    amoclingoSearch::prepare(solver);
    // the bound decreases: the clauses learnt under a bound must not outlive its call
    amoclingoSearch::useVolatileClauses(solver);

    const int64_t maxValue = amoclingoSearch::maxPossibleValue(amomaximize);
    const int64_t minValue = std::min(amoclingoSearch::minPossibleValue(amomaximize), maxValue);
    int64_t unsatBound = maxValue + 1;
    size_t step = 0;
    AmosumClingoAssignment* best = nullptr;

    amomaximize->updateBound(maxValue);
    while(true){
        auto [answerset, result] = solver.solve();

        if(result.isSat()){
            best = amoclingoSearch::acceptModel(answerset, result, best);
            step = 0;                                       // back to the last UNSAT bound
        }
        else if(result.isUnsat()){
            unsatBound = amomaximize->bound;
            // UNSAT where the #amomaximize constrains nothing: no answer set
            if(best == nullptr && unsatBound <= minValue) return result;
        }
        else return best != nullptr ? best->result : result;   // interrupted: best (if any) stays "unknown"

        if(best != nullptr && best->amosumCost + 1 >= unsatBound) return amoclingoSearch::optimum(best);

        // unsatBound - p(step), clamped to the smallest bound still open (above the last model's value, or the minimum
        // possible value before any model)
        const int64_t lower = best != nullptr ? best->amosumCost + 1 : minValue;
        const int64_t decrement = progression.at(step);
        const int64_t next = (unsatBound - lower >= decrement) ? unsatBound - decrement : lower;
        ++step;
        #ifdef __debug__
            LOG(fiore::LogLevel::INFO, "UNSAT-SAT (", progression.expression, "): UNSAT from ", unsatBound, ", lower ", lower, ", next bound ", next);
        #endif
        amomaximize->updateBound(next);
    }
}
