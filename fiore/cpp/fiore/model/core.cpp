#include "core.h"
#include "solver.h"



namespace fiore {

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
