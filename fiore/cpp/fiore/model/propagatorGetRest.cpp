#include "propagatorGetRest.h"



namespace fiore {

PropagatorGetRestLiterals::PropagatorGetRestLiterals(std::string name, ClingoSolver* solver,  Vector<const Literal*>* targets): ClingoPropagator(name, solver), rest(nullptr), targets(targets){
    this->init = &initImpPropagatorGetSoftFacts;
}

void PropagatorGetRestLiterals::initFirstTime(clingo_propagate_init_t *_init){
    solver->initializer->init(_init, this->solver);
    rest = &this->solver->initializer->programAtomRestNoFact;
    #ifdef __debug__
        LOG(LogLevel::INFO, "targets: ",this->solver->initializer->globalSoftSolvingLiterals);
        LOG(LogLevel::INFO, "rest: ",rest);
    #endif
    this->firstTime = false;
}

bool initImpPropagatorGetSoftFacts(clingo_propagate_init_t *_init, ClingoPropagator* propagator){
    #ifdef __debug__
        auto start = startTimer();
    #endif
    if(propagator->clearIfNotEnabledInInit(_init)){return true;}
    PropagatorGetRestLiterals* getterRest = static_cast<PropagatorGetRestLiterals*>(propagator);
    if(getterRest->firstTime)
        getterRest->initFirstTime(_init);
    #ifdef __debug__
        double elasped = elapsedTime(start);
        propagator->increaseStat("initTime",elasped);
    #endif

    
    return true;
}

} // namespace fiore
