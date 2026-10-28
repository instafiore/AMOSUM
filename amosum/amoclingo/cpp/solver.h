#pragma once
#include <iostream>
#include <clingo.h>
#include <vector>
#include <memory>
#include <unordered_set>
#include "fiore/utils/logger.h"
#include "fiore/model/program.h"
#include "fiore/model/solver.h"
#include "amosum/core/cpp/utility.h"
#include "amosum/core/cpp/program.h"
#include "amosum/core/cpp/search/algorithms.h"
#include "amosum/amoclingo/cpp/propagator.h"

// amosum on clingo (mirrored by amoclingo/py/solver.py): AmosumClingoSolver grounds the program and registers one
// AmoclingoPropagator per aggregate; AmosumClingoBackend runs the searches of core/cpp/search on it

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

// the searches on clingo: one solve call per step, the bound of the #amomaximize read live by its propagator
struct AmosumClingoBackend: public AmosumSearchBackend{
    explicit AmosumClingoBackend(AmosumClingoSolver& solver) noexcept;
    AmosumConstraint* amomaximize() noexcept override;
    std::pair<AmosumClingoAssignment*, fiore::clingo::ClingoResult> solve() noexcept override;
    void setBound(int64_t bound) noexcept override;
    void useVolatileClauses() noexcept override;
private:
    AmosumClingoSolver& solver;
};
