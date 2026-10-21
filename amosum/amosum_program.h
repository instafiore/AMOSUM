#pragma once
#include <iostream>
#include <clingo.h>
#include "utility.h"
#include <vector>
#include <memory>
#include "fiore/utils/logger.h" 
#include "fiore/model/program.h" 
#include <unordered_set>


struct AmosumConstraint: public fiore::PBConstraint{
    std::string id;                                     // aggregate id: last argument of __group__
    std::string type;                                   // ge_amo, ge_eo, le_amo, le_eo, amomaximize (from __aux__)
    bool ge;                                            // __lb__ (>=) or __ub__ (<=)
    std::vector<Group*> groups;
    std::unique_ptr<fiore::PerfectNHash<clingo_literal_t, size_t>> groupFunction; // literal -> index in groups
    std::vector<clingo_literal_t> groupsAtoms;          // the __group__ atoms themselves

    AmosumConstraint(const fiore::Program* program, int64_t bound, fiore::Vector<const fiore::Literal*>* literals, fiore::UMNWeightFunction* weightFunction) noexcept
        : fiore::PBConstraint(program, bound, literals, weightFunction), ge(true), ownedLiterals(literals), ownedWeightFunction(weightFunction) {
        // normalize();
    }

    inline std::string toString() const noexcept override{ std::ostringstream out; out<<"AmosumConstraint("<<id<<", "<<type<<", "<<ownedLiterals<<", "<<preprocessedSum<<", "<<bound<< ")"; return out.str();};

    ~AmosumConstraint() override{ for(Group* g: groups) delete g; }

private:
    std::unique_ptr<fiore::Vector<const fiore::Literal*>> ownedLiterals;
    std::unique_ptr<fiore::UMNWeightFunction> ownedWeightFunction;
    
    // void normalize();
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