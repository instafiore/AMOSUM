#pragma once
#include <limits>
#include <unordered_map>
#include "fiore/model/propagatorPBImp.h"
#include "amoclingo/cpp/amoclingo_propagator.h"

// port of Group (utility.h) over solver literals: one copy per thread, modified during search
struct AmosumThreadGroup{
    static constexpr size_t NONE_INDEX = std::numeric_limits<size_t>::max();
    
    std::vector<clingo_literal_t> ordL;                   // literals ordered by weight (the ones fixed at level 0 are removed)
    std::unordered_map<clingo_literal_t, size_t> ordI;    // literal -> index in ordL
    size_t countUndef;                                    // undefined literals of the group
    size_t maxUndI;                                       // index in ordL of the maximum undefined literal (ge)
    size_t minUndI;                                       // index in ordL of the minimum undefined literal (le)
    const std::vector<clingo_literal_t>* movedLiterals;   // weight 0 in this group: see AmosumSolvingGroup
    size_t movedNotFalse;                                 // moved literals not false
    clingo_literal_t trueLiteral;                         // SETTINGS::NONE_LITERAL if no literal of the group is true
    bool trueIsMoved;                                     // the true literal is a moved one (weight 0 here)

    explicit AmosumThreadGroup(const AmosumSolvingGroup& g) noexcept;

        inline clingo_literal_t maxUnd() const noexcept { return maxUndI == NONE_INDEX ? SETTINGS::NONE_LITERAL : ordL[maxUndI]; }
    inline clingo_literal_t minUnd() const noexcept { return minUndI == NONE_INDEX ? SETTINGS::NONE_LITERAL : ordL[minUndI]; }
    inline void setMaxMin(const clingo_literal_t l, const bool max) noexcept { (max ? maxUndI : minUndI) = l != SETTINGS::NONE_LITERAL ? ordI.at(l) : NONE_INDEX; }

    // (new maximum/minimum undefined literal, previous one); assumingUnd is considered undefined
    std::pair<clingo_literal_t, clingo_literal_t> updateMax(const fiore::PerfectInterpretation& I, bool all, bool update, clingo_literal_t assumingUnd = SETTINGS::NONE_LITERAL) noexcept;
    std::pair<clingo_literal_t, clingo_literal_t> updateMin(const fiore::PerfectInterpretation& I, bool all, bool update, clingo_literal_t assumingUnd = SETTINGS::NONE_LITERAL) noexcept;
    inline std::pair<clingo_literal_t, clingo_literal_t> update(const fiore::PerfectInterpretation& I, bool max, bool all, bool update, clingo_literal_t assumingUnd = SETTINGS::NONE_LITERAL) noexcept {
        return max ? updateMax(I, all, update, assumingUnd) : updateMin(I, all, update, assumingUnd);
    }

    // level 0: l is fixed for the whole search (port of simplifyLiterals)
    void remove(clingo_literal_t l, const fiore::PerfectInterpretation& I) noexcept;
};

// port of AmoSumPropagator over solver literals, one per solver thread
struct AmosumPropagator: public fiore::PropagatorPBConstraintImplementation{

    explicit AmosumPropagator(AmoclingoPropagator* abs) noexcept;

    void onLiteralsUndefined(clingo_propagate_control_t const *control, clingo_literal_t const *literalIds, size_t size, size_t dl = 0) noexcept override;
    const std::vector<clingo_literal_t>& getReasonForLiteral(const clingo_literal_t& literalId) const noexcept override;
    // value of the aggregate for the true literals (with the constant): on a total assignment, the value of the model
    inline int64_t currentValue() const noexcept { return currentSum; }

protected:
    std::pair<bool, bool> updatePhase(clingo_literal_t const *literalIds, size_t size, size_t dl = 0) noexcept override;
    std::vector<clingo_literal_t>& propagatePhase(clingo_propagate_control_t* control = nullptr) noexcept override;

private:
    AmoclingoPropagator* const prop;
    const bool ge;
    const bool amo;
    // member literal -> index of its group in this thread, NO_GROUP once removed at level 0: with one thread it is
    // the propagator's groupIndex itself (no other reader after init), otherwise a copy owned here
    std::unique_ptr<fiore::PerfectNHash<clingo_literal_t, size_t>> ownedGroupIndex;
    fiore::PerfectNHash<clingo_literal_t, size_t>* groupIndex;
    std::vector<AmosumThreadGroup> groups;
    std::unique_ptr<fiore::PerfectInterpretation> I;
    int64_t currentSum;                 // weights of the true literals, with the constant
    int64_t mpsValue;                   // max (ge) / min (le) possible sum, with the constant
    int64_t maxDiff;                    // largest change of the possible sum by one group (old lazy threshold)
    size_t decisionLevel;
    bool nextPhase;
    int64_t lastBound;                  // bound at the last update: a new bound (LSU, between solve calls) triggers propagation
    std::vector<clingo_literal_t> watchesToRemove;
    std::unique_ptr<fiore::PerfectNHash<clingo_literal_t, std::vector<clingo_literal_t>>> reasons;   // derived literal (or BOTTOM) -> its reason
    std::unique_ptr<fiore::PerfectNHash<clingo_literal_t, int64_t>> slack;   // min/minfly, ge: weight that can still be left out of the reason of a derived literal

    inline int64_t bound() const noexcept { return prop->pbConstraint->bound; }
    inline bool amoLe() const noexcept { return amo && !ge; }
    inline int64_t mps() const noexcept { return amoLe() ? currentSum : mpsValue; }
    inline int64_t weightOf(const clingo_literal_t l) const noexcept { return l == SETTINGS::NONE_LITERAL ? 0 : prop->weight->at(l); }
    inline bool isMemberHere(const clingo_literal_t l) const noexcept { return (size_t)std::abs(l) < prop->N && groupIndex->at(l) != AmoclingoPropagator::NO_GROUP; }
    inline size_t groupOf(const clingo_literal_t l) const noexcept {
        if(isMemberHere(l)) return groupIndex->at(l);
        if(isMemberHere(fiore::not_(l))) return groupIndex->at(fiore::not_(l));
        return AmoclingoPropagator::NO_GROUP;
    }
    int64_t contribution(const AmosumThreadGroup& g) const noexcept;
    bool lazyCondition() const noexcept;
    std::vector<clingo_literal_t> initPropagation() noexcept override;

    bool onMemberTrue(clingo_literal_t l) noexcept;
    bool onMemberFalse(clingo_literal_t l) noexcept;
    bool onMovedTrue(size_t gi, clingo_literal_t l) noexcept;
    bool onMovedFalse(size_t gi) noexcept;
    void onMemberUndefined(clingo_literal_t l) noexcept;
    void simplify(clingo_literal_t l) noexcept;
    void decide(AmosumThreadGroup& g, clingo_literal_t c) noexcept;    // level 0: c true decides g, its literals leave it

    void derive(clingo_literal_t l, int64_t mpsH = 0) noexcept;     // mpsH: possible sum assuming the opposite of l (ge, for min/minfly)
    void propagateGeAmo() noexcept;
    void propagateGeEo() noexcept;
    void propagateLeAmo(const clingo_assignment_t* assignment) noexcept;   // le_amo: falsity not watched, read from the solver
    void propagateLeEo() noexcept;
    // reasons: the literals (false in the current assignment) that make the derivation valid, for the groups other than
    // `excluded` (the derived literal's group, NO_GROUP for a conflict)
    void explainMaxOfGroups(clingo_literal_t derived, size_t excluded) noexcept; // ge: max possible sum
    void explainMaxWithout(clingo_literal_t derived, clingo_literal_t second, const AmosumThreadGroup& g) noexcept;  // ge: ml must be true
    void explainMinOfGroups(clingo_literal_t derived, size_t excluded) noexcept; // le_eo: min possible sum
    void explainTrueOfGroups(clingo_literal_t derived, size_t excluded) noexcept; // le_amo: current sum

    
};