#include "amosum/core/cpp/search/algorithms.h"
#include "fiore/utils/logger.h"

fiore::clingo::ClingoResult binarySearch(AmosumSearchBackend& backend) noexcept{
    AmosumConstraint* amomaximize = backend.amomaximize();
    if(amomaximize == nullptr) return amosumSearch::solveOnce(backend);

    backend.useVolatileClauses();

    const int64_t maxValue = amosumSearch::maxPossibleValue(amomaximize);
    const int64_t minValue = std::min(amosumSearch::minPossibleValue(amomaximize), maxValue);
    int64_t lo = minValue - 1;
    int64_t unsatBound = maxValue + 1;
    AmosumClingoAssignment* best = nullptr;
    fiore::clingo::ClingoResult lastUnsat;                  

    while(true){
        if(lo + 1 >= unsatBound){
            if(best != nullptr) return amosumSearch::optimum(best);
            return lastUnsat;
        }
        const int64_t mid = lo + (unsatBound - lo) / 2;
        #ifdef __debug__
            LOG(fiore::LogLevel::INFO, "Binary search: SAT up to ", lo, ", UNSAT from ", unsatBound, ", bound ", mid);
        #endif
        backend.setBound(mid);

        auto [answerset, result] = backend.solve();
        if(result.isSat()){
            best = amosumSearch::acceptModel(answerset, result, best);
            lo = best->amosumCost;                          
        }
        else if(result.isUnsat()){ unsatBound = mid; lastUnsat = result; }
        else return best != nullptr ? best->result : result;   
    }
}
