#include "propagatorPBAbstraction.h"
#include "propagatorPBImp.h"
#include "propagatorPBImpLe.h"
#include "solver.h"


namespace fiore {

PropagatorPBConstraint::~PropagatorPBConstraint(){
    for(PropagatorPBConstraintImplementation* p :implementations) if(p != nullptr) delete p;
}

void PropagatorPBConstraint::initFirstTime(clingo_propagate_init* _init) noexcept{
    ClingoPropagator::initFirstTime(_init);

    auto it = std::max_element(localSolvingLiterals.begin(), localSolvingLiterals.end(), [](const Literal* l1, const Literal* l2){ return std::abs(l1->id) < std::abs(l2->id);});

    if(it == localSolvingLiterals.end()) return;

    maxSolvingLiteralId = std::abs((*it)->id);
    weightSolvingLiterals.reset(new PerfectWeightFunction(maxSolvingLiteralId + 1));

    for(const SolvingLiteral* solvingLiteral: localSolvingLiterals){
        const std::unordered_set<const Literal*>& programLiterals = solvingLiteral->programLiterals;
        int64_t weightSolvingLiteral = sum(*pbConstraint->weightFunction, programLiterals);
        weightSolvingLiterals->set(solvingLiteral->id, weightSolvingLiteral);
    }

    // Setting weight facts
    const SolvingLiteral* solvingLiteralFact = (SolvingLiteral*) this->solver->initializer->mapSlitId2Slit->get(1);
    const std::unordered_set<const Literal*>& facts = solvingLiteralFact->programLiterals;
    int64_t weightFacts = sum(*pbConstraint->weightFunction, facts); 
    weightSolvingLiterals->set(1, weightFacts);

    std::sort(localSolvingLiterals.begin(), localSolvingLiterals.end(), 
    [this](const SolvingLiteral* s1, const SolvingLiteral* s2){
        return weightSolvingLiterals->at(s1->id) < weightSolvingLiterals->at(s2->id);
    });

    #ifdef __debug__
        LOG(LogLevel::INFO, "Solving literals of ",pbConstraint," are: ", localSolvingLiterals, " - PropagatorPBConstraint::initFirstTime");
    #endif

    for(size_t ni = 0; ni < solver->initializer->nt; ++ni){
        implementations.emplace_back(new PropagatorPBConstraintImplementationLe(this));
    }
}

bool PropagatorPBConstraint::addClause(const clingo_propagate_control_t *control, const std::vector<clingo_literal_t>& clause) noexcept {
    bool result_add_clause;
    handleError(clingo_propagate_control_add_clause((clingo_propagate_control*) control, clause.data(), clause.size(), clingo_clause_type_volatile, &result_add_clause)) ;

    #ifdef __debug__
        LOG(LogLevel::DEBUG_1,"Adding Clause [slits]: ", this->solvingLiteralIdsToSolvingLiterals(clause));
    #endif

    // propagation must return immediately, there is a conflict
    if (!result_add_clause){
        #ifdef __debug__
            LOG(LogLevel::DEBUG_1,"Conflict with clause [slits]: ", this->solvingLiteralIdsToSolvingLiterals(clause));
            LOG(LogLevel::DEBUG_1, this->solvingLiteralIdToSolvingLiteral(clause[0])," is undef: ",clingo::isUndef(clingo_propagate_control_assignment(control), clause[0]), " clause[0]: ",clause[0]);
            LOG(LogLevel::DEBUG_1, this->solvingLiteralIdToSolvingLiteral(clause[0])," is true: ",clingo::isTrue(clingo_propagate_control_assignment(control), clause[0]));
            LOG(LogLevel::DEBUG_1, this->solvingLiteralIdToSolvingLiteral(clause[0])," is false: ",clingo::isFalse(clingo_propagate_control_assignment(control), clause[0]));
        #endif
        return true ;
    }

    bool result_propagate;
    handleError(clingo_propagate_control_propagate((clingo_propagate_control*)control, &result_propagate)) ;
    
    if (!result_propagate){ 
        #ifdef __debug__
            LOG(LogLevel::DEBUG_1,"Conflict with propagating [slits]: ", clause);
        #endif
        return true ;
    }   

    return false ;
}

bool PropagatorPBConstraint::addClausesPropagatedLitIds(const clingo_propagate_control_t *control, const std::vector<clingo_literal_t>& propagatedLiteralIds) noexcept{
    size_t td = clingo_propagate_control_thread_id((const clingo_propagate_control_t *)control);

    const PropagatorPBConstraintImplementation* imp = implementations[td];
    for(size_t si = 0; si < propagatedLiteralIds.size(); ++si){  
        const clingo_literal_t& literalId = propagatedLiteralIds[si];
        const std::vector<clingo_literal_t>& r = imp->getReasonForLiteral(literalId);
        std::vector<clingo_literal_t> clause;
        clause.reserve(r.size() + 1);
        clause.push_back(literalId);
        extendVector(clause, r);
        if(addClause(control, clause))return true;
    }
    return false;
}

bool initImpDecisionWeakPropagatorAbstraction(clingo_propagate_init_t *init, ClingoPropagator* propagator){
    #ifdef __debug__
        auto start = startTimer();
    #endif
    PropagatorPBConstraint* pbProp = static_cast<PropagatorPBConstraint*>(propagator);

    #ifdef __debug__
        LOG(LogLevel::INFO_1, "Called ",pbProp->name," - initImpDecisionWeakPropagatorAbstraction");
    #endif

    if(pbProp->clearIfNotEnabledInInit(init)) 
        return true;

    const bool& isFirstTime = pbProp->firstTime;
    if(isFirstTime) 
        pbProp->initFirstTime(init);
    
    if(pbProp->localSolvingLiterals.size() == 0)
        return true;
    
    if(isFirstTime){
        #ifdef __debug__
            LOG(LogLevel::INFO, pbProp," adding watches - initImpDecisionWeakPropagatorAbstraction");
        #endif
        for(const SolvingLiteral* slit: pbProp->localSolvingLiterals){
            #ifdef __debug__
                LOG(LogLevel::INFO, pbProp->name," adding watch @ ", slit, " - initImpDecisionWeakPropagatorAbstraction");
            #endif
            const clingo_literal_t &watchedLiteral = slit->id;
            handleError(clingo_propagate_init_add_watch(init, watchedLiteral));
            // handleError(clingo_propagate_init_add_watch(init, -watchedLiteral));
            pbProp->watchedLiterals.push_back(watchedLiteral);
        }
        #ifdef __debug__
            LOG(LogLevel::INFO, "Adding watch to fact 1");
        #endif
        handleError(clingo_propagate_init_add_watch(init, 1));
        pbProp->watchedLiterals.push_back(1);
    }
    #ifdef __debug__
        propagator->firstTime = false;    
        double elasped = elapsedTime(start);
        propagator->increaseStat("initTime", elasped);
    #endif
    return true;
}

bool propagateImpDecisionWeakPropagatorAbstraction(clingo_propagate_control_t *control, clingo_literal_t const *changes, size_t size, ClingoPropagator* propagator){
    #ifdef __debug__
        auto start = startTimer();
    #endif
    PropagatorPBConstraint* pbProp = static_cast<PropagatorPBConstraint*>(propagator);

    const clingo_assignment_t* assignment =  clingo_propagate_control_assignment(control);
    const size_t dl = clingo_assignment_decision_level(assignment);
    const size_t td = clingo_propagate_control_thread_id(control);

    PropagatorPBConstraintImplementation*& imp = pbProp->implementations[td];
    const std::vector<clingo_literal_t>& propagatedLiteralIds =  imp->onLiteralsTrue(control, changes, size, dl);
    if(pbProp->addClausesPropagatedLitIds(control, propagatedLiteralIds)) return true;
    #ifdef __debug__
        double elasped = elapsedTime(start);
        propagator->increaseStat("propagateTime", elasped);
    #endif
    return true;
}

void undoImpDecisionWeakPropagatorAbstraction(clingo_propagate_control_t const *control, clingo_literal_t const *changes, size_t size, ClingoPropagator* propagator){
    #ifdef __debug__
        auto start = startTimer();
    #endif
    PropagatorPBConstraint* pbProp = static_cast<PropagatorPBConstraint*>(propagator);
    const clingo_assignment_t* assignment =  clingo_propagate_control_assignment(control);
    const size_t dl = clingo_assignment_decision_level(assignment);
    const size_t td = clingo_propagate_control_thread_id(control);

    PropagatorPBConstraintImplementation*& imp = pbProp->implementations[td];
    imp->onLiteralsUndefined(control, changes, size, dl);
    #ifdef __debug__
        double elasped = elapsedTime(start);
        propagator->increaseStat("undoTime", elasped);
    #endif
}

} // namespace fiore
