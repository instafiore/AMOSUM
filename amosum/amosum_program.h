#pragma once
#include <iostream>
#include <clingo.h>
#include "utility.h"
#include <vector>
#include <memory>
#include "fiore/utils/logger.h" 
#include "fiore/model/program.h" 
#include <unordered_set>


// a group of an AmosumConstraint over program literals (from the grounding): at most one member true (exactly one for EO)
struct ProgramGroup{
    std::string id;
    std::vector<std::pair<const fiore::Literal*, int64_t>> members;   // program literal, weight (0: counted in another group)
    // the group of the "-" element (¬__aux__, weight = bound): it changes only when the aggregate's body does, so it is left
    // out of the lazy threshold (otherwise the threshold is always met)
    bool aux = false;
};
struct AmosumConstraint: public fiore::PBConstraint{
    // the aggregate is described by `groups` only: PBConstraint's literals and weight function are empty (owned here,
    // as PBConstraint keeps raw pointers) and must not be used for amosum
    std::string id;                                     // aggregate id: last argument of __group__
    std::string type;                                   // ge_amo, ge_eo, le_amo, le_eo, amomaximize (from __aux__)
    bool ge;                                            // __lb__ (>=) or __ub__ (<=)
    std::vector<ProgramGroup> groups;                   // after the program level normalization (AmosumGrounder)
    std::vector<clingo_literal_t> groupsAtoms;          // the __group__ atoms themselves

    AmosumConstraint(const fiore::Program* program, int64_t bound, fiore::Vector<const fiore::Literal*>* literals, fiore::UMNWeightFunction* weightFunction) noexcept
        : fiore::PBConstraint(program, bound, literals, weightFunction), ge(true), ownedLiterals(literals), ownedWeightFunction(weightFunction) {}

    inline std::string toString() const noexcept override{ std::ostringstream out; out<<"AmosumConstraint("<<id<<", "<<type<<", "<<groups.size()<<" groups, "<<(ge ? ">= " : "<= ")<<bound<< ")"; return out.str();};
private:
    std::unique_ptr<fiore::Vector<const fiore::Literal*>> ownedLiterals;
    std::unique_ptr<fiore::UMNWeightFunction> ownedWeightFunction;      // unused
};

class AmosumProgram: public fiore::Program{
public:
    std::vector<AmosumConstraint*> amosumConstraints;   // non-owning: the constraints are owned by pBConstraints

    struct AmosumGrounder: fiore::Program::ProgramGrounder{
        fiore::Program* ground(const std::vector<std::string>& ASPInputs) override;
    protected:
        fiore::Program* createProgram() const override;
    };

    AmosumProgram() = default;
};