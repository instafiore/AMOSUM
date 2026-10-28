#pragma once
#include <vector>
#include "args-parser/args-parser/all.hpp"
#include <chrono>
#include <unordered_map>
#include <unordered_set>
#include <assert.h>
#include <ostream>
#include <algorithm>
#include <sstream>
#include "constants.h"
#include <compare>
#include <any>
#include <cmath>
#include "utilityCommon.h"
#include <cstddef>


namespace fiore {

inline Clock::time_point wallTime;


struct UMNWeightFunction;

struct Cluster{

    Vector<const Literal*> literals;

    Cluster(const Vector<const Literal*>&& literals)noexcept: literals(literals){}

    inline std::string toString() const noexcept{ 
        std::ostringstream oss;
        oss<<std::string("Cluster(")<<literals<<std::string(")");
        return oss.str();
    }
};

int64_t meanCluster(const Cluster& cluster, UMNWeightFunction* weightFunction);

template<typename D>
struct WeightFunction: public NHash<int32_t, int64_t, D, const Literal*, std::vector<const Literal*>>{

    WeightFunction(size_t N, int64_t defaultValue = 0)noexcept: NHash<int32_t, int64_t, D, const Literal*, std::vector<const Literal*>>(N, std::move(defaultValue)), weightNone(0){}

    const int64_t& getUsingHash(const size_t& key) const override{
        if(key == CONSTANTS::INFPlus64) return weightNone;
        return NHash<int32_t, int64_t, D, const Literal*, std::vector<const Literal*>>::getUsingHash(key);
    }

private:
    const int64_t weightNone;
};


struct PerfectWeightFunction: public WeightFunction<std::vector<int64_t>>{
    
    PerfectWeightFunction(size_t N, int64_t defaultValue = 0)noexcept:  WeightFunction<std::vector<int64_t>>(N, std::move(defaultValue)){
        this->_data = new std::vector<int64_t>(this->N, defaultValue);
    }
};

struct UMNWeightFunction: public WeightFunction<std::unordered_map<size_t, int64_t>>{
    UMNWeightFunction(size_t N, int64_t defaultValue = 0)noexcept:  WeightFunction<std::unordered_map<size_t, int64_t>>(N, std::move(defaultValue)){
        this->_data = new std::unordered_map<size_t, int64_t>();
    }
};


std::ostream& operator<<(std::ostream& out, const Cluster& cluster);

template<typename D, typename A = std::vector<const Literal*>>
inline int64_t sum(WeightFunction<D>& weight,  const A& softLiterals) noexcept{
    int64_t res = 0;
    for(const Literal* literal: softLiterals){
        int64_t w = weight.at(literal->id);
        if(w > 0 && w != CONSTANTS::INFPlus64)
            NOOP;
        res += w == CONSTANTS::INFPlus64 ? 0 : w;
    }
    return res;
}

void shuffleWithinClusters(Vector<const Literal*>& targets, const std::vector<size_t>& indexes, std::mt19937& gen) noexcept;

[[nodiscard]] constexpr size_t factorial(size_t n) noexcept {
    size_t f = 1;
    for (size_t k = 2; k <= n; ++k) {
        f *= k;
    }
    return f;
}

inline void startWallTime() noexcept{ wallTime = startTimer();}
inline double getWallTime() noexcept{ return elapsedTime(wallTime);}

void cleanup() noexcept;

} // namespace fiore

#include "utility.tpp"
