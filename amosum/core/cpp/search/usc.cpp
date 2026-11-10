#include "amosum/core/cpp/search/algorithms.h"
#include "fiore/utils/logger.h"

fiore::clingo::ClingoResult usc(AmosumSearchBackend&  backend) noexcept{
    AmosumConstraint* amomaximize = backend.amomaximize();
    if(amomaximize == nullptr) return amosumSearch::solveOnce(backend);

    // TODO

    return fiore::clingo::ClingoResult(); // None for now
}