#pragma once
#include "../utils/utility.h"
#include "propagator.h"
#include "solver.h"



namespace fiore {

bool initImpPropagatorGetSoftFacts(clingo_propagate_init_t *init, ClingoPropagator* prop);

struct PropagatorGetRestLiterals: public ClingoPropagator{

    PropagatorGetRestLiterals(std::string name, ClingoSolver* solver, Vector<const Literal*>* targets = nullptr);
    
    inline const Vector<const Literal*>& programLiterals() override { return *solver->program->restLiterals.get(); }
    inline const Vector<const Literal*>& softLiterals() { return targets != nullptr ? *targets : *solver->program->softLiterals.get(); }
    
    friend bool initImpPropagatorGetSoftFacts(clingo_propagate_init_t *init, ClingoPropagator* prop);
    void initFirstTime(clingo_propagate_init_t *_init) override;
    Vector<const Literal*>* rest;
private:
    Vector<const Literal*>* targets;
};

} // namespace fiore
