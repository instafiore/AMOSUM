#include "propagatorPBImp.h"



namespace fiore {

 PropagatorPBConstraintImplementation::PropagatorPBConstraintImplementation(PropagatorPBConstraint* abs): abs(abs), currentCost(abs->pbConstraint->preprocessedCost){}

const std::vector<clingo_literal_t>& PropagatorPBConstraintImplementation::onLiteralsTrue(clingo_propagate_control_t *control, clingo_literal_t const *literalIds, size_t size, size_t dl)noexcept{

    const std::pair<bool, bool> propagateOrConflict = this->updatePhase(literalIds,size,dl);
    const bool &progagatePhaseActivation = propagateOrConflict.first;
    const bool &conflict = propagateOrConflict.second;
    S.clear();
    if(conflict){
        #ifdef __debug__
            LOG(LogLevel::DEBUG_3, "Conflict - PropagatorPBConstraintImplementationLe::onLiteralsTrue");
            if(dl == 0) LOG(LogLevel::CRITICAL, "incosistent at level 0 - PropagatorPBConstraintImplementationLe::simplifyAtLevelZero");
        #endif
        S.push_back(CONSTANTS::BOTTOM);
        return S;
    }
    
    return progagatePhaseActivation ? this->propagatePhase(control) : S;
}

} // namespace fiore
