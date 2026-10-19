#pragma once
#include "../utils/utility.h"
#include "../clingo/utilityClingo.h"



namespace fiore {

class LowerBound{
protected:
    int64_t value;

public:
    LowerBound(): value(CONSTANTS::INFMinus64){}
    inline void increase(const int64_t& x) noexcept{ if(value != CONSTANTS::INFMinus64) value += x; else value = x; }
    inline void decrease(const int64_t& x) noexcept{ if(value != CONSTANTS::INFMinus64) value -= x; }
    inline void set(const int64_t& x) noexcept{ value = x; }
    inline int64_t getValue() noexcept { return value; };
};

class LowerBoundOBV: public LowerBound{

protected:
    // std::unordered_map<size_t, int64_t> progression;
    std::unique_ptr<PerfectHash<size_t, int64_t>> progression;
    size_t progressionSize;

public:
    LowerBoundOBV(): LowerBound(), progressionSize(0){}
    inline int64_t get(const size_t& i) const noexcept { return progression->get(i); }
    inline int64_t get() const noexcept { return progression->get(progressionSize); }
    inline void updateProgression(const size_t& i, const int64_t x) noexcept { if(i >= progressionSize) progressionSize = i+1;  progression->set(i, x); }
    size_t getShufflingPoint() const noexcept;
    size_t getMaxIncreasePoint() const noexcept;
    size_t getShufflingPointBasedOnPercentage(const float& p = 0.7) const noexcept;
    void setMaxValueAssumptionIndex(const size_t& maxValueIndex) noexcept;
    inline size_t getMaxValueAssumptionIndex() const noexcept { return progressionSize; }
    void clearProgressionFromK(const size_t k = 0) noexcept;
};

class Core;
std::ostream& operator<<(std::ostream& out, const Core& core);

class Core{
    friend std::ostream& operator<<(std::ostream& out, const Core& core);
    private:
        std::unordered_set<clingo_literal_t> core;
        std::vector<clingo_literal_t> coreIterable;
        
    public:
        
        Core(){}
        Core(std::unordered_set<clingo_literal_t>&& c): core(std::move(c)), coreIterable(core.begin(), core.end()){}
        inline bool has(const clingo_literal_t& l) const noexcept { return this->core.find(l) != this->core.end(); }
        inline void clear() noexcept { core.clear(); coreIterable.clear();}
        inline void add(const clingo_literal_t& l) noexcept{ if(core.emplace(l).second) coreIterable.push_back(l); }

        inline size_t size() const noexcept{ return this->core.size(); }
        const clingo_literal_t& at(const size_t& i) const noexcept { return coreIterable[i]; }
        inline bool isBlocked(const std::vector<const Literal*>& assumptions) const noexcept{ size_t inside=0; for(const Literal* l: assumptions) if(has(l->id)) ++inside; return inside == size(); }
};

class CoreLowerBound{
public:
    void addTarget(const clingo_literal_t target, const int64_t weight) noexcept;
    bool addForced(const clingo_literal_t target) noexcept;
    bool addCore(const Core& core) noexcept;

    int64_t value() const noexcept{ return lb; }

private:
    std::unordered_map<clingo_literal_t, int64_t> residual;
    int64_t lb = 0;
};

} // namespace fiore
