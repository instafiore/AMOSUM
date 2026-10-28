#pragma once
#include <algorithm>
#include <cstdint>
#include <memory>
#include <string>
#include "fiore/clingo/utilityClingo.h"
#include "fiore/model/assigment.h"
#include "amosum/core/cpp/program.h"

// Search algorithms on the #amomaximize value (mirrored by core/py/search/), on any AmosumSearchBackend: the clingo
// one is amoclingo/cpp/solver.h (AmosumClingoBackend).
// Each one returns the result of the search; the best answer set is the pool's best known one
// (fiore::AnswersetsPool::getBestKnownAnswerSet), which is also what signalHandler prints on SIGINT/SIGTERM.
// Without #amomaximize every algorithm is a single solve call.


// --progression: how far the bound moves at step n = 0, 1, 2, ... of satUnsat / unsatSat, as an expression in n with integers,
// n, + - * ^ and parentheses: 1 (linear, the default), 2 (+2), 2^n, n^2, 3*n+1, ... A value below 1 counts as 1
// (n^2 at n = 0), values are capped at 2^62
class Progression{
public:
    explicit Progression(const std::string& expression);    // std::invalid_argument on a malformed expression
    int64_t at(size_t n) const noexcept;
    // 1 at every step: the bound of satUnsat never goes back, so its clauses can stay learnt
    bool isLinear() const noexcept { return linear; }
    const std::string expression;

    struct Node;
private:
    std::shared_ptr<const Node> root;
    bool linear;
};


// what a search needs from a solver: one solve call with the current bound of the #amomaximize
struct AmosumSearchBackend{
    virtual ~AmosumSearchBackend() = default;
    virtual AmosumConstraint* amomaximize() noexcept = 0;           // nullptr: no #amomaximize
    // one solve call: the model (an AmosumClingoAssignment in the pool) if SAT, and the result
    virtual std::pair<AmosumClingoAssignment*, fiore::clingo::ClingoResult> solve() noexcept = 0;
    virtual void setBound(int64_t bound) noexcept = 0;              // the bound of the #amomaximize for the next calls
    virtual void useVolatileClauses() noexcept = 0;                 // the bound may decrease: clauses must not outlive a call
};


// SAT-UNSAT (upwards; LSU with p = 1): from the minimum possible value, after a model of value v the bound is v + p(step) (step+1 at each
// model), never at a bound known UNSAT nor above the maximum possible value; an UNSAT call goes back to v + 1 with
// step 0. With p = 1: each model v sets the bound to v + 1 until UNSAT (the last model is optimal)
fiore::clingo::ClingoResult satUnsat(AmosumSearchBackend& backend, const Progression& progression) noexcept;

// UNSAT-SAT (downwards; LUS with p = 1): from the maximum possible value, after an UNSAT call at bound u the bound is u - p(step) (step+1 at
// each UNSAT call), never at a value known SAT nor below the minimum possible value; a model goes back to u - 1 with
// step 0. With p = 1: down by 1 until the first model, which is optimal
fiore::clingo::ClingoResult unsatSat(AmosumSearchBackend& backend, const Progression& progression) noexcept;

// binary search: between the best model's value v (every bound <= v is SAT) and the smallest bound known UNSAT, each
// call tries the middle bound; a model raises v to its value, an UNSAT call lowers the UNSAT bound to the middle one
fiore::clingo::ClingoResult binarySearch(AmosumSearchBackend& backend) noexcept;


// helpers shared by the algorithms
namespace amosumSearch {

    // largest / smallest value of the #amomaximize: at most one member per group, possibly none (the aux group, of
    // weight bound = 0, is left out)
    inline int64_t maxPossibleValue(const AmosumConstraint* constraint) noexcept{
        int64_t value = 0;
        for(const ProgramGroup& g: constraint->groups){
            if(g.aux) continue;
            int64_t best = 0;
            for(const auto& [literal, weight]: g.members) best = std::max(best, weight);
            value += best;
        }
        return value;
    }
    inline int64_t minPossibleValue(const AmosumConstraint* constraint) noexcept{
        int64_t value = 0;
        for(const ProgramGroup& g: constraint->groups){
            if(g.aux) continue;
            int64_t worst = 0;
            for(const auto& [literal, weight]: g.members) worst = std::min(worst, weight);
            value += worst;
        }
        return value;
    }

    // a model of backend.solve(): the new best answer set (the previous one is deleted), printed as not yet optimal
    inline AmosumClingoAssignment* acceptModel(fiore::ClingoAssignment* answerset, const fiore::clingo::ClingoResult& result, AmosumClingoAssignment* best) noexcept{
        AmosumClingoAssignment* a = static_cast<AmosumClingoAssignment*>(answerset);
        a->result = result;
        if(best != nullptr) fiore::AnswersetsPool::getInstance()->deleteAnswerset(best);
        a->setAsBestKnown();
        a->result.unkown();             // a better model may exist
        a->printAssignment();
        return a;
    }

    // best is proven optimal
    inline fiore::clingo::ClingoResult optimum(AmosumClingoAssignment* best) noexcept{
        best->result.optimum();
        return best->result;
    }

    // without #amomaximize: one solve call (a #minimize is optimized by clingo inside it)
    inline fiore::clingo::ClingoResult solveOnce(AmosumSearchBackend& backend) noexcept{
        auto [a, result] = backend.solve();
        if(!result.isSat()) return result;
        a->result = result;
        a->setAsBestKnown();
        return a->result;
    }
}
