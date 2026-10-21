#include "propagatorPBImpLe.h"
#include "solver.h"


namespace fiore {

std::pair<bool, bool> PropagatorPBConstraintImplementationLe::updatePhase(clingo_literal_t const *literalIds, size_t size, size_t dl) noexcept{
    #ifdef __debug__
        LOG(LogLevel:: INFO, "changes: ", this->abs->solvingLiteralIdsToSolvingLiterals(literalIds, size), " level: ", dl, " - PropagatorPBConstraintImplementationLe::updatePhase");
    #endif
    currentLevel = dl;
    const int64_t &incW = computeCost(literalIds, size, nullptr, true);
    currentCost += incW;
    const int64_t &ub = abs->pbConstraint->bound;
    #ifdef __debug__
        int64_t actualCostC = actualCost();
        assert(actualCostC == currentCost);
        LOG(LogLevel::INFO, "Current cost: ", currentCost, " actualCostC: ",actualCostC, " upper bound: ", ub, " - PropagatorPBConstraintImplementationLe::updatePhase");
    #endif

    return std::make_pair<bool,bool>(true, currentCost > ub);
}


int64_t PropagatorPBConstraintImplementationLe::actualCost(){
    int64_t c = this->abs->weightSolvingLiterals->at(1);
    for(const clingo_literal_t &l: this->currentFalseSolvingLiteralIds){
        c += this->abs->weightSolvingLiterals->at(not_(l));
    }
    return c;
}

std::vector<clingo_literal_t>& PropagatorPBConstraintImplementationLe::propagatePhase(clingo_propagate_control_t* control) noexcept{
    const int64_t& ub = abs->pbConstraint->bound;

    const clingo_assignment_t* assignment = clingo_propagate_control_assignment(control);

    int32_t i = maxUndefined;
    while(i >= 0 && !clingo::isUndef(assignment, abs->ownedSolvingLiterals[i]->id)) --i;
    if(i != maxUndefined){
        if(maxUndefinedTrail.empty() || maxUndefinedTrail.back().first != currentLevel) maxUndefinedTrail.emplace_back(currentLevel, maxUndefined);
        maxUndefined = i;
    }

    // same scan as before, starting from the first undefined literal
    for(; i >= 0; --i){
        const clingo_literal_t& slitId = abs->ownedSolvingLiterals[i]->id;
        if(!clingo::isUndef(assignment, slitId)) continue;
        if(possibleSum(slitId) > ub) S.push_back(not_(slitId));
        else break;   // sorted by weight: no lighter literal can exceed ub either
    }

    #ifdef __debug__
        LOG(LogLevel::INFO, "propagated literal: ",this->abs->solvingLiteralIdsToSolvingLiterals(S)," - PropagatorPBConstraintImplementationLe::propagatePhase");
    #endif

    #ifdef __debug__
        const int32_t n = abs->ownedSolvingLiterals.size();
        std::vector<clingo_literal_t> expected;
        for(int32_t k = n-1; k >= 0; --k){
            const clingo_literal_t& s = abs->ownedSolvingLiterals[k]->id;
            if(!clingo::isUndef(assignment, s)) continue;
            if(possibleSum(s) > ub) expected.push_back(not_(s)); else break;
        }
        assert(expected == S);
    #endif
    return S;
}


void PropagatorPBConstraintImplementationLe::onLiteralsUndefined(clingo_propagate_control_t const *control, clingo_literal_t const *solvingLiteralIds, size_t size, size_t dl)noexcept{
    #ifdef __debug__
        LOG(LogLevel:: INFO, "undefined literals: ", this->abs->solvingLiteralIdsToSolvingLiterals(solvingLiteralIds, size), " level: ", dl, " - PropagatorPBConstraintImplementationLe::onLiteralsUndefined");
    #endif

    #ifdef __debug__
        // const clingo_assignment_t* assigment = clingo_propagate_control_assignment(control);
        // std::vector<const SolvingLiteral*> trueLits = trueLitearls(assigment, this->abs->solvingLiterals);
        // std::vector<const SolvingLiteral*> falseLits = falseLitearls(assigment, this->abs->solvingLiterals);
        // std::vector<const SolvingLiteral*> undefLits = undefinedLitearls(assigment, this->abs->solvingLiterals);
        // LOG(LogLevel::DEBUG, "True literals: ",trueLits, "- PropagatorPBConstraintImplementationLe::onLiteralsUndefined");
        // LOG(LogLevel::DEBUG, "False literals: ",falseLits, "- PropagatorPBConstraintImplementationLe::onLiteralsUndefined");
        // LOG(LogLevel::DEBUG, "Undefined literals: ",undefLits, "- PropagatorPBConstraintImplementationLe::onLiteralsUndefined");
    #endif

    int64_t decreaseCost = 0;

    // std::__1::vector<int> copy = currentFalseSolvingLiteralIds;
    for(size_t i = 0; i < size; ++i){
        // bool problem = currentFalseSolvingLiteralIds.size() == 0;
        // if(problem){
        //     log(LogLevel::DEBUG, "currentFalseSolvingLiteralIds: ",copy);
        // }
        currentFalseSolvingLiteralIds.pop_back();
        clingo_literal_t removedLiteralId = solvingLiteralIds[i];
        decreaseCost += abs->weightSolvingLiterals->at(removedLiteralId);
    }
    currentCost -= decreaseCost;
    while(!maxUndefinedTrail.empty() && maxUndefinedTrail.back().first >= dl){
        maxUndefined = maxUndefinedTrail.back().second;
        maxUndefinedTrail.pop_back();
    }
}

} // namespace fiore
