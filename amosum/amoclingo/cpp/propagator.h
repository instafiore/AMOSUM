#pragma once
#include "fiore/model/propagatorPBConstraint.h"
#include "amosum/core/cpp/program.h"
#include "amosum/core/cpp/propagator.h"

// The clingo propagator of one amosum aggregate (mirrored by amoclingo/py/propagator.py): it maps the program groups to
// solver literals (buildSolvingGroups), watches them and runs one AmosumPropagator (core) per solver thread
struct AmoclingoPropagator: public fiore::PropagatorPBConstraint{

    AmoclingoPropagator(std::string name, fiore::ClingoSolver* solver, AmosumConstraint* constraint, const std::string& reason = "nomin", const std::string& lazy = SETTINGS::FALSE_STR, const clingo_clause_type_t clauseType = clingo_clause_type_learnt) noexcept;

    AmosumConstraint* const constraint;
    const bool ge;                              // >= (ge_*, amomaximize) or <= (le_*)
    const bool eo;                              // EO groups
    const AmosumOptions options;                // --reason, --lazy
    AmosumSolvingGroups solvingGroups;          // built at the first init

protected:
    fiore::PropagatorPBConstraintImplementation* createImplementation() noexcept override;
    void initFirstTime(clingo_propagate_init* _init) noexcept override;

private:
    std::vector<std::unique_ptr<fiore::SolvingLiteral>> memberSolvingLiterals;   // what ownedSolvingLiterals points to
};
