#include "propagatorGetDerivedFromAss.h"



namespace fiore {

PropagatorGetDerivedFromAss::PropagatorGetDerivedFromAss(std::string name, ClingoSolver* solver, Vector<const Literal*>* targets): ClingoPropagator(name, solver), targets(targets){
    this->init = &initImpPropagatorGetDerivedFromAss;
    this->propagate = &propagateImpGetDerivedFromAss;
}


bool initImpPropagatorGetDerivedFromAss(clingo_propagate_init_t *_init, ClingoPropagator* propagator){
    #ifdef __debug__
        auto start = startTimer();
    #endif
    if(propagator->clearIfNotEnabledInInit(_init)){return true;}
    PropagatorGetDerivedFromAss* propGetDerFromAss = static_cast<PropagatorGetDerivedFromAss*>(propagator);
    
    if(propGetDerFromAss->firstTime){
        propGetDerFromAss->initFirstTime(_init);
        size_t NSlits = propGetDerFromAss->solver->initializer->maxSoftSolverLiteralId+1;
        size_t NPlits = propGetDerFromAss->solver->program->maxLiteralId+1;
        propGetDerFromAss->targetsDerivedFromAssumptions.reset(new PerfectNSet<clingo_literal_t>(NPlits));
        propGetDerFromAss->mapTargetsDerivedFromAssumptionsSlits.reset(new PerfectNHash<clingo_literal_t, std::vector<clingo_literal_t>>(NSlits));
        propGetDerFromAss->assumptionsSlitIds.reset(new PerfectNSet<clingo_literal_t>(NSlits));

        // TODO: Understand if doing one at init or each init
        for(const SolvingLiteral* slit: propGetDerFromAss->ownedSolvingLiterals){
            const clingo_literal_t &watchedLiteral = slit->id;
            propGetDerFromAss->watchedLiterals.push_back(watchedLiteral);
            handleError(clingo_propagate_init_add_watch(_init, watchedLiteral));
            #ifdef __debug__
                LOG(LogLevel::DEBUG, propGetDerFromAss->name," adding watch @ ", watchedLiteral, " - initImpPropagatorGetDerivedFromAss");
            #endif
            handleError(clingo_propagate_init_add_watch(_init, not_(watchedLiteral)));
            #ifdef __debug__
                LOG(LogLevel::DEBUG, propGetDerFromAss->name," adding watch @ ", not_(watchedLiteral), " - initImpPropagatorGetDerivedFromAss");
            #endif
            propGetDerFromAss->watchedLiterals.push_back(not_(watchedLiteral));
        }
    }

    for(const clingo_literal_t &l: propGetDerFromAss->assumptions){
        clingo_literal_t slit;
        clingo_propagate_init_solver_literal(_init, l, &slit);
        propGetDerFromAss->assumptionsSlitIds->add(slit);
    }

    #ifdef __debug__
        std::string derivedFromAssumptionString = propGetDerFromAss->targetsDerivedFromAssumptions->toStringSet(propGetDerFromAss->solver->program->mapLiteralId2Literal.get()); 
        LOG(LogLevel::INFO,"Derived from assumptions: ",derivedFromAssumptionString);
    #endif

    // remember that watches are persistent between calls
    // for(const SolvingLiteral* slit: propGetDerFromAss->solvingLiterals){
    //     const clingo_literal_t &watchedLiteral = slit->id;
    //     bool alreadyInDerivedSet = false;
    //     for(const Literal* plit: slit->programLiterals){
    //         if(propGetDerFromAss->targetsDerivedFromAssumptions->contains(plit->id) || propGetDerFromAss->targetsDerivedFromAssumptions->contains(not_(plit->id))) alreadyInDerivedSet = true; 
    //         break;
    //     }
    //     if(alreadyInDerivedSet){
    //         #ifdef __debug__
    //             LOG(LogLevel::DEBUG, propGetDerFromAss->name," not adding watch @ ", watchedLiteral, " because already in set of derived lits from assumptions - initImpPropagatorGetDerivedFromAss");    
    //         #endif
    //         handleError(clingo_propagate_init_remove_watch(_init, not_(watchedLiteral)));
    //         continue;
    //     }
    //     #ifdef __debug__
    //         LOG(LogLevel::DEBUG, propGetDerFromAss->name," adding watch @ ", watchedLiteral, " - initImpPropagatorGetDerivedFromAss");
    //     #endif
    //     propGetDerFromAss->watchedLiterals.push_back(watchedLiteral);
    //     handleError(clingo_propagate_init_add_watch(_init, watchedLiteral));
    //     #ifdef __debug__
    //         LOG(LogLevel::DEBUG, propGetDerFromAss->name," adding watch @ ", not_(watchedLiteral), " - initImpPropagatorGetDerivedFromAss");
    //     #endif
    //     handleError(clingo_propagate_init_add_watch(_init, not_(watchedLiteral)));
    //     propGetDerFromAss->watchedLiterals.push_back(not_(watchedLiteral));
    // }

    #ifdef __debug__
        double elasped = elapsedTime(start);
        propagator->increaseStat("initTime",elasped);
    #endif

    propGetDerFromAss->firstTime = false;
    return true;
}


bool propagateImpGetDerivedFromAss(clingo_propagate_control_t *control, clingo_literal_t const *changes, size_t size, ClingoPropagator* propagator){
    #ifdef __debug__
        auto start = startTimer();
    #endif

    PropagatorGetDerivedFromAss* prop = static_cast<PropagatorGetDerivedFromAss*>(propagator);

    const clingo_assignment_t* assignment =  clingo_propagate_control_assignment(control);
    const size_t dl = clingo_assignment_decision_level(assignment);

    clingo_literal_t decisionLiteralId = CONSTANTS::TOP; 
    if(dl > 0) handleError(clingo_assignment_decision(assignment, dl, &decisionLiteralId));
    const size_t rootLevel = clingo_assignment_root_level(assignment);
    #ifdef __debug__
        LOG(LogLevel:: DEBUG,"root level: ", rootLevel," level: ", dl," decisionLit: ",prop->solver->initializer->mapSoftSlitId2SoftSlit->get(decisionLiteralId)," changes: ", prop->solvingLiteralIdsToSolvingLiterals(changes, size),", - propagateImpGetDerivedFromAss::updatePhase");
        LOG(LogLevel:: DEBUG,"root level: ", rootLevel," level: ", dl," decisionLit: ",prop->solver->initializer->mapSoftSlitId2SoftSlit->get(decisionLiteralId)," assumptionsSlit: ", prop->assumptionsSlitIds->toStringSet(prop->solver->initializer->mapSoftSlitId2SoftSlit.get()), " - propagateImpGetDerivedFromAss::updatePhase");
        LOG(LogLevel::DEBUG,"root level: ", rootLevel," level: ", dl," decisionSLit: ",decisionLiteralId," decisionLit: ",prop->solver->initializer->mapSoftSlitId2SoftSlit->get(decisionLiteralId)," assumptions: ", prop->assumptions, " - propagateImpGetDerivedFromAss::updatePhase");
    #endif
    if(dl > rootLevel) return true;

    if(decisionLiteralId == CONSTANTS::TOP || prop->assumptionsSlitIds->contains(decisionLiteralId)){
        for(size_t i = 0; i < size; ++i){
            clingo_literal_t l = changes[i];
            const SolvingLiteral* slit = (SolvingLiteral*)prop->solver->initializer->mapSoftSlitId2SoftSlit->get(l);
            for(const Literal* plit: slit->programLiterals){
                if(prop->targetsDerivedFromAssumptions->contains(plit->id)) continue;
                #ifdef __debug__
                    LOG(LogLevel::DEBUG,"Derived ",prop->solver->program->mapLiteralId2Literal->get(plit->id)," from assumptions: ", prop->assumptions, " with decision literal: ", decisionLiteralId);
                #endif 
                prop->targetsDerivedFromAssumptions->add(plit->id);
                prop->mapTargetsDerivedFromAssumptionsSlits->add(decisionLiteralId, plit->id);
            }
        }  
    }

    #ifdef __debug__
        double elasped = elapsedTime(start);
        propagator->increaseStat("propagateTime", elasped);
    #endif
    return true;
}


void PropagatorGetDerivedFromAss::addAssumption(const clingo_literal_t& plit){ this->assumptions.push_back(plit); }
void PropagatorGetDerivedFromAss::addAssumptionAndSet(const clingo_literal_t& plit, const std::vector<clingo_literal_t>&& derivedFromL){
    addAssumption(plit);
    const clingo_literal_t slitAssumption = this->solver->initializer->mapSoftPlit2SoftSlit[plit];
    mapTargetsDerivedFromAssumptionsSlits->set(slitAssumption, derivedFromL);
    for(const clingo_literal_t& d: derivedFromL) targetsDerivedFromAssumptions->add(d);
}

void PropagatorGetDerivedFromAss::removeAssumption(const clingo_literal_t& plit){
    #ifdef __debug__
        LOG("Removed assumption: ",this->solver->program->mapLiteralId2Literal->get(plit));
    #endif
    const clingo_literal_t slitAssumption = this->solver->initializer->mapSoftPlit2SoftSlit[plit];
    std::vector<clingo_literal_t>& derivedLiterals = this->mapTargetsDerivedFromAssumptionsSlits->get(slitAssumption);
    for(const clingo_literal_t& derived: derivedLiterals){
        targetsDerivedFromAssumptions->remove(derived);
    }
    assumptionsSlitIds->remove(slitAssumption); 
    derivedLiterals.clear();
}


void PropagatorGetDerivedFromAss::setDerivedFromAssumption(const clingo_literal_t& plit, std::vector<clingo_literal_t>&& derivedLiterals) noexcept{
    const clingo_literal_t slitAssumption = this->solver->initializer->mapSoftPlit2SoftSlit[plit];
    this->mapTargetsDerivedFromAssumptionsSlits->set(slitAssumption, derivedLiterals);
}

void PropagatorGetDerivedFromAss::removeFromIToLastAssumption(const size_t i) noexcept{
    const size_t& n = assumptions.size();
    for(size_t k=i; k < n; ++k){
        removeAssumption(assumptions.get(k));
    }
    assumptions.resize(i);
}


void PropagatorGetDerivedFromAss::clearAssumptions(){ 
    removeFromIToLastAssumption(0);
}

const std::vector<clingo_literal_t>& PropagatorGetDerivedFromAss::derivedFromAssumption(const clingo_literal_t& plit) noexcept{
    const clingo_literal_t slitAssumption = this->solver->initializer->mapSoftPlit2SoftSlit[plit];
    return this->mapTargetsDerivedFromAssumptionsSlits->get(slitAssumption);
}

} // namespace fiore
