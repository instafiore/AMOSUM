#include "amosum_solver.h"


fiore::Program* AmosumClingoSolver::groundProgram(const std::vector<std::string>& ASPInputs) noexcept{
    AmosumProgram::AmosumGrounder grounder;
    return grounder.ground(ASPInputs);
}