#include "amosum/core/cpp/search/algorithms.h"
#include "fiore/utils/logger.h"

fiore::clingo::ClingoResult satUnsat(AmosumSearchBackend& backend, const Progression& progression) noexcept{
    AmosumConstraint* amomaximize = backend.amomaximize();
    if(amomaximize == nullptr) return amosumSearch::solveOnce(backend);

    if(!progression.isLinear()) backend.useVolatileClauses();

    int64_t unsatBound = amosumSearch::maxPossibleValue(amomaximize) + 1;
    size_t step = 0;
    AmosumClingoAssignment* best = nullptr;

    backend.setBound(amosumSearch::minPossibleValue(amomaximize));
    while(true){
        auto [answerset, result] = backend.solve();

        if(result.isSat()){
            best = amosumSearch::acceptModel(answerset, result, best);
        }
        else if(result.isUnsat()){
            if(best == nullptr) return result;              
            unsatBound = amomaximize->bound;                
            step = 0;                                       
        }
        else return best != nullptr ? best->result : result;   

        const int64_t value = best->amosumCost;
        if(value + 1 >= unsatBound) return amosumSearch::optimum(best);

        const int64_t increment = progression.at(step);
        const int64_t next = (unsatBound - 1 - value >= increment) ? value + increment : unsatBound - 1;
        ++step;
        #ifdef __debug__
            LOG(fiore::LogLevel::INFO, "SAT-UNSAT (", progression.expression, "): value ", value, ", UNSAT from ", unsatBound, ", next bound ", next);
        #endif
        backend.setBound(next);
    }
}
