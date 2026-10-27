#include "amoclingo_algorithms.h"
#include <string>
#include "fiore/model/assigment.h"
#include "fiore/utils/logger.h"

fiore::clingo::ClingoResult lsu(AmosumClingoSolver& solver) noexcept{
    solver.setNoConflictLimits();
    solver.clearAssumptions();
    solver.disableRestDetection();
    solver.disableGetDerivedFromAssDetection();
    solver.setCollectUnsatCores(false);

    fiore::AnswersetsPool* pool = fiore::AnswersetsPool::getInstance();
    AmosumConstraint* amomaximize = static_cast<AmosumProgram*>(solver.program.get())->amomaximizeConstraint;
    AmosumClingoAssignment* best = nullptr;

    while(true){
        auto [answerset, result] = solver.solve();

        if(!result.isSat()){
            if(best == nullptr) return result;              // UNSAT (or interrupted) before any model
            if(result.isUnsat()) best->result.optimum();    // nothing better than best: it is optimal
            return best->result;                            // interrupted: best stays "unknown"
        }

        // AmosumClingoSolver::createAssignment builds AmosumClingoAssignment
        AmosumClingoAssignment* a = static_cast<AmosumClingoAssignment*>(answerset);
        a->result = result;
        if(best != nullptr) pool->deleteAnswerset(best);
        best = a;
        best->setAsCurrentOptimum();

        // nothing to maximize: one call (a #minimize is optimized by clingo inside it)
        if(amomaximize == nullptr) return best->result;

        best->result.unkown();                              // a better model may exist
        #ifdef __debug__
            LOG(fiore::LogLevel::INFO, "LSU: model of value ", best->amosumCost);
        #endif
        best->printAssignment();
        amomaximize->updateBound(best->amosumCost + 1);     // next model must be strictly better
    }
}