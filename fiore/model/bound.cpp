#include "bound.h"
#include "solver.h"



namespace fiore {

size_t LowerBoundOBV::getShufflingPoint() const noexcept{

    const std::string pointStrategy = ArgParse::getInstance()->get(CONSTANTS::LOWER_BOUND_SHUFFLING_SELECTING_POINT_KEY);
    if(pointStrategy == CONSTANTS::LOWER_BOUND_SHUFFLING_SELECTING_POINT_MaxIncrease) return getMaxIncreasePoint();
    
    const float p = stof(pointStrategy);
    return getShufflingPointBasedOnPercentage(p);
}

void LowerBoundOBV::setMaxValueAssumptionIndex(const size_t& maxValueIndex) noexcept {
    if(progression.get() == nullptr || progression->size() < maxValueIndex + 1)
        progression.reset(new PerfectHash<size_t, int64_t>(maxValueIndex + 1, CONSTANTS::INFMinus64));
}

size_t LowerBoundOBV::getMaxIncreasePoint() const noexcept{
    size_t maxI = 0;
    int64_t maxValue = 0;
    int64_t previousLb = 0;
    for(size_t i=0; i < progressionSize; ++i){
        int64_t lbValue = progression->get(i);
        assert(lbValue != CONSTANTS::INFMinus64);
        if(lbValue == CONSTANTS::INFMinus64) continue;
        int64_t diff = lbValue - previousLb;
        assert(diff >= 0);
        if(diff > maxValue){
            maxValue = diff;
            maxI = i;
        }
        previousLb = lbValue;
    }
    return maxI;
}

size_t LowerBoundOBV::getShufflingPointBasedOnPercentage(const float& p) const noexcept{
    setExitCode(CONSTANTS::ERROR_CODE, true, "getShufflingPointBasedOnPercentage not implemented yet");
    return 0;
}

void LowerBoundOBV::clearProgressionFromK(const size_t k) noexcept{
    // for(size_t i=k; i < progressionSize; ++i){
    //     progression->set(i, CONSTANTS::INFMinus64);
    // }
    progressionSize = k;
}


void CoreLowerBound::addTarget(const clingo_literal_t target, const int64_t weight) noexcept{ residual[target] = weight; }

// target true in every solution (e.g. forced by the facts): pay its whole weight once
bool CoreLowerBound::addForced(const clingo_literal_t target) noexcept{
    auto it = residual.find(target);
    if(it == residual.end() || it->second <= 0) return false;
    lb += it->second;
    it->second = 0;
    return true;
}

bool CoreLowerBound::addCore(const Core& core) noexcept{
    if(core.size() == 0) return false;
    int64_t m = INT64_MAX;
    for(size_t i = 0; i < core.size(); ++i){
        auto it = residual.find(not_(core.at(i)));
        if(it == residual.end()) return false;
        m = std::min(m, it->second);
    }
    if(m <= 0) return false;
    for(size_t i = 0; i < core.size(); ++i) residual[not_(core.at(i))] -= m;
    lb += m;
    return true;
}

} // namespace fiore
