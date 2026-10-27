#include "amosum_solver.h"


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
    }
    #ifdef __debug__
        LOG(fiore::LogLevel::INFO, "Registered ", amosumPropagators.size(), " amosum propagators (reason ", reason, ", lazy ", lazy, ")");
    #endif
}
