#pragma once
#include <iostream>
#include <clingo.h>
#include "utility.h"
#include <vector>
#include <memory>
#include "fiore/utils/logger.h"
#include "fiore/model/program.h"
#include "fiore/model/solver.h"
#include <unordered_set>
#include "amosum_program.h"
#include "amoclingo/cpp/amoclingo_propagator.h"

class AmosumClingoSolver: public fiore::ClingoSolver{
protected:
    fiore::Program* groundProgram(const std::vector<std::string>& ASPInputs) noexcept override;
public:
    // reason, lazy: --reason and --lazy, the same for every amosum constraint
    AmosumClingoSolver(std::string reason = "nomin", std::string lazy = SETTINGS::FALSE_STR): ClingoSolver(),  maximizer(nullptr), reason(std::move(reason)), lazy(std::move(lazy)){}

    // grounds the program (ClingoSolver::init) and registers one AmoclingoPropagator per AmosumConstraint
    void init(const std::vector<std::string>& ASPInputs) noexcept;
    fiore::ClingoAssignment* createAssignment(const clingo_model_t* model) override{ return new AmosumClingoAssignment(static_cast<AmosumProgram*>(program.get()), model);}
    std::vector<AmoclingoPropagator*> amosumPropagators;    // non-owning: ClingoSolver::propagators owns them
    AmoclingoPropagator* maximizer;
private:
    const std::string reason;
    const std::string lazy;
};
