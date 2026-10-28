#include "amosum/amoclingo/cpp/solver.h"


fiore::Program* AmosumClingoSolver::groundProgram(const std::vector<std::string>& ASPInputs) noexcept{
    AmosumProgram::AmosumGrounder grounder;
    return grounder.ground(ASPInputs);
}

void AmosumClingoSolver::init(const std::vector<std::string>& ASPInputs) noexcept{
    fiore::ClingoSolver::init(ASPInputs);
    // groundProgram is overridden: the program is an AmosumProgram, with its constraints read from the grounding
    AmosumProgram* amosumProgram = static_cast<AmosumProgram*>(program.get());
    for(AmosumConstraint* constraint: amosumProgram->amosumConstraints){
        AmoclingoPropagator* propagator = new AmoclingoPropagator("amosum " + constraint->id, this, constraint, reason, lazy);
        registerPropagator(propagator);         // registered on program->ctl, owned by ClingoSolver::propagators
        amosumPropagators.push_back(propagator);
        if(propagator->constraint->type == "amomaximize"){
            assert(maximizer == nullptr);
            maximizer = propagator;
        }
    }
    #ifdef __debug__
        LOG(fiore::LogLevel::INFO, "Registered ", amosumPropagators.size(), " amosum propagators (reason ", reason, ", lazy ", lazy, ")");
    #endif
}

// ----- AmosumClingoBackend -----

AmosumClingoBackend::AmosumClingoBackend(AmosumClingoSolver& solver) noexcept: solver(solver){
    // plain solve calls: no assumptions, cores, rest or derived-literal detection
    solver.setNoConflictLimits();
    solver.clearAssumptions();
    solver.disableRestDetection();
    solver.disableGetDerivedFromAssDetection();
    solver.setCollectUnsatCores(false);
}

AmosumConstraint* AmosumClingoBackend::amomaximize() noexcept{
    return static_cast<AmosumProgram*>(solver.program.get())->amomaximizeConstraint;
}

std::pair<AmosumClingoAssignment*, fiore::clingo::ClingoResult> AmosumClingoBackend::solve() noexcept{
    auto [answerset, result] = solver.solve();
    // AmosumClingoSolver::createAssignment builds AmosumClingoAssignment
    return {static_cast<AmosumClingoAssignment*>(answerset), result};
}

void AmosumClingoBackend::setBound(int64_t bound) noexcept{
    amomaximize()->updateBound(bound);
}

void AmosumClingoBackend::useVolatileClauses() noexcept{
    // only the #amomaximize propagator: the other constraints' bounds never change, their clauses stay learnt
    if(solver.maximizer != nullptr) solver.maximizer->updateClauseTypeGlobal(clingo_clause_type_volatile);
}
