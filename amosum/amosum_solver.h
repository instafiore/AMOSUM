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

class AmosumClingoSolver: public fiore::ClingoSolver{
protected:
    fiore::Program* groundProgram(const std::vector<std::string>& ASPInputs) noexcept override;
public:
    AmosumClingoSolver(): ClingoSolver(){}
};