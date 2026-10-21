#pragma once
#include <limits>
#include "fiore/model/propagatorPBAbstraction.h"
#include "../../amosum_program.h"

// a group of an AmosumConstraint over solver literals: shared by the threads, never modified during search
struct AmosumSolvingGroup{
    std::string id;
    std::vector<clingo_literal_t> literals;     // ordered by (weight, literal)
    std::vector<clingo_literal_t> movedLiterals;
    bool aux;                                   // the group of the "-" element (ProgramGroup::aux): left out of the lazy threshold
};

struct AmoclingoPropagator: public fiore::PropagatorPBConstraint{

    static constexpr size_t NO_GROUP = std::numeric_limits<size_t>::max();

    AmoclingoPropagator(std::string name, fiore::ClingoSolver* solver, AmosumConstraint* constraint, const std::string& reason = "nomin", const std::string& lazy = SETTINGS::FALSE_STR) noexcept;

    AmosumConstraint* const constraint;
    const bool ge;                              // >= (ge_*, amomaximize) or <= (le_*)
    const bool eo;                              // EO groups
    std::vector<AmosumSolvingGroup> solvingGroups;
    std::unique_ptr<fiore::PerfectNHash<clingo_literal_t, int64_t>> weight;     // member solver literal -> sum of the weights of its elements
    std::unique_ptr<fiore::PerfectNHash<clingo_literal_t, size_t>> groupIndex;  // member solver literal -> index of its owner group
    std::unordered_map<clingo_literal_t, std::vector<size_t>> movedInto;        // moved solver literal -> indexes of the groups where it has weight 0
    bool hasMovedLiterals = false;                                              // movedInto not empty: skips its lookups on every change
    int64_t constant;                           // weight of the groups decided by a fact
    size_t N;                                   // max solver atom + 1
    // --reason (>= only, as in the old code):
    //   ijcai:      all the false literals of the derived literal's group (first version)
    //   nomin:      from the second maximum of the derived literal's group
    //   min/minfly: nomin minimized greedily; the old min gives the same reasons after the construction, minfly during it
    //   cmin:       not implemented (as nomin)
    const bool fullTrueReason;                  // ijcai
    const bool minimizeOnTheFly;                // min, minfly
    // --lazy: when to propagate (a violated bound is a conflict in every mode)
    //   false, hybrid: when a single group can still bring the possible sum across the bound (threshold from the
    //                  groups; with hybrid the minimum weight of a group is used for AMO too, as in the old code)
    //   true, float p: when lb/mps (ge) or mps/ub (le) reaches 1 or p
    const bool lazyActive;                      // lazy != false
    const bool lazyThreshold;                   // false or hybrid
    const double lazyPerc;                      // true: 1, float: its value

    inline bool isMember(const clingo_literal_t l) const noexcept { return (size_t)std::abs(l) < N && groupIndex->at(l) != NO_GROUP; }
    inline bool isMoved(const clingo_literal_t l) const noexcept { return hasMovedLiterals && movedInto.find(l) != movedInto.end(); }

protected:
    fiore::PropagatorPBConstraintImplementation* createImplementation() noexcept override;
    void initFirstTime(clingo_propagate_init* _init) noexcept override;

private:
    std::vector<std::unique_ptr<fiore::SolvingLiteral>> memberSolvingLiterals;   // what ownedSolvingLiterals points to
};