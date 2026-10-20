#pragma once
#include "../utils/utility.h"
#include "../clingo/utilityClingo.h"



namespace fiore {

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
