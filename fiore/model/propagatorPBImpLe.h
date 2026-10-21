#pragma once
#include "propagatorPBImp.h"


namespace fiore {

struct PropagatorPBConstraintImplementationLe : public PropagatorPBConstraintImplementation{

    PropagatorPBConstraintImplementationLe(PropagatorPBConstraint* abs): PropagatorPBConstraintImplementation(abs), maxUndefined((int32_t)abs->ownedSolvingLiterals.size() - 1){} 
    // const std::vector<clingo_literal_t> simplifyAtLevelZero() override;
    std::pair<bool, bool> updatePhase(clingo_literal_t const *literalIds, size_t size, size_t dl) noexcept override;
    std::vector<clingo_literal_t>& propagatePhase(clingo_propagate_control_t* control) noexcept override;
    void onLiteralsUndefined(clingo_propagate_control_t const *control, clingo_literal_t const *solvingLitearlsIds, size_t size, size_t dl = 0) noexcept override;

    int64_t actualCost();
private:
    int32_t maxUndefined;
    std::vector<std::pair<size_t, int32_t>> maxUndefinedTrail;
    size_t currentLevel = 0;
};

} // namespace fiore
