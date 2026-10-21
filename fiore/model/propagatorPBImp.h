#pragma once
#include "propagatorPBAbstraction.h"



namespace fiore {

struct PropagatorPBConstraintImplementation{

    PropagatorPBConstraintImplementation(PropagatorPBConstraint* abs);

    const std::vector<clingo_literal_t>& onLiteralsTrue(clingo_propagate_control_t *control, clingo_literal_t const *literalIds, size_t size, size_t dl = 0) noexcept;
    virtual inline const std::vector<clingo_literal_t>& getReasonForLiteral(const clingo_literal_t& solvingLiteralId) const noexcept { return currentFalseSolvingLiteralIds; }
    virtual void onLiteralsUndefined(clingo_propagate_control_t const *control, clingo_literal_t const *solvingLitearlsIds, size_t size, size_t dl = 0) noexcept = 0;

    virtual ~PropagatorPBConstraintImplementation() = default;
    std::vector<clingo_literal_t> currentFalseSolvingLiteralIds;
protected:
    PropagatorPBConstraint* abs;
    int64_t currentCost;
    std::vector<clingo_literal_t> S;

    virtual std::pair<bool, bool> updatePhase(clingo_literal_t const *literalIds, size_t size, size_t dl = 0) noexcept = 0;
    virtual std::vector<clingo_literal_t>& propagatePhase(clingo_propagate_control_t* control = nullptr) noexcept = 0;

    inline int64_t computeCost(clingo_literal_t const *literalIds, size_t size, PerfectWeightFunction* weight = nullptr, bool updateInterpretation = false) noexcept{
        if(weight == nullptr) weight = abs->weightSolvingLiterals.get();
        int64_t cost = 0;
        for(size_t i = 0; i < size; ++i){
            const clingo_literal_t& literalId = literalIds[i];
            if(updateInterpretation && literalId != CONSTANTS::TOP) currentFalseSolvingLiteralIds.push_back(not_(literalId));
            const int64_t& w = weight->at(literalId);
            cost += w;
        }
        return cost;
    }

    inline int64_t possibleSum(clingo_literal_t literalId) noexcept{ return currentCost + computeCost(&literalId, 1, nullptr, false);}
};

} // namespace fiore
