#include "amosum/core/cpp/search/algorithms.h"
#include "fiore/utils/logger.h"

fiore::clingo::ClingoResult unsatSat(AmosumSearchBackend& backend, const Progression& progression) noexcept{
    AmosumConstraint* amomaximize = backend.amomaximize();
    if(amomaximize == nullptr) return amosumSearch::solveOnce(backend);

    backend.useVolatileClauses();

    const int64_t maxValue = amosumSearch::maxPossibleValue(amomaximize);
    const int64_t minValue = std::min(amosumSearch::minPossibleValue(amomaximize), maxValue);
    int64_t unsatBound = maxValue + 1;
    size_t step = 0;
    AmosumClingoAssignment* best = nullptr;

    backend.setBound(maxValue);
    while(true){
        auto [answerset, result] = backend.solve();

        if(result.isSat()){
            best = amosumSearch::acceptModel(answerset, result, best);
            step = 0;                                      
        }
        else if(result.isUnsat()){
            unsatBound = amomaximize->bound;
            if(best == nullptr && unsatBound <= minValue) return result;
        }
        else return best != nullptr ? best->result : result;   
        if(best != nullptr && best->amosumCost + 1 >= unsatBound) return amosumSearch::optimum(best);

        const int64_t lower = best != nullptr ? best->amosumCost + 1 : minValue;
        const int64_t decrement = progression.at(step);
        const int64_t next = (unsatBound - lower >= decrement) ? unsatBound - decrement : lower;
        ++step;
        #ifdef __debug__
            LOG(fiore::LogLevel::INFO, "UNSAT-SAT (", progression.expression, "): UNSAT from ", unsatBound, ", lower ", lower, ", next bound ", next);
        #endif
        backend.setBound(next);
    }
}
