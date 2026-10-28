#include "propagatorPBConstraint.h"
#include "propagatorPBImp.h"
#include "propagatorPBImpLe.h"
#include "solver.h"


namespace fiore {

PropagatorPBConstraint::~PropagatorPBConstraint(){
    for(PropagatorPBConstraintImplementation* p :implementations) if(p != nullptr) delete p;
}

void PropagatorPBConstraint::initFirstTime(clingo_propagate_init* _init) noexcept{
    ClingoPropagator::initFirstTime(_init);

    auto it = std::max_element(ownedSolvingLiterals.begin(), ownedSolvingLiterals.end(), [](const Literal* l1, const Literal* l2){ return std::abs(l1->id) < std::abs(l2->id);});

    if(it == ownedSolvingLiterals.end()) return;

    maxSolvingLiteralId = std::abs((*it)->id);
    weightSolvingLiterals.reset(new PerfectWeightFunction(maxSolvingLiteralId + 1));

    for(const SolvingLiteral* solvingLiteral: ownedSolvingLiterals){
        const std::unordered_set<const Literal*>& programLiterals = solvingLiteral->programLiterals;
        int64_t weightSolvingLiteral = sum(*pbConstraint->weightFunction, programLiterals);
        weightSolvingLiterals->set(solvingLiteral->id, weightSolvingLiteral);
    }

    // Setting weight facts
    const SolvingLiteral* solvingLiteralFact = (SolvingLiteral*) this->solver->initializer->mapSoftSlitId2SoftSlit->get(fiore::CONSTANTS::TOP);
    const std::unordered_set<const Literal*>& facts = solvingLiteralFact->programLiterals;
    int64_t weightFacts = sum(*pbConstraint->weightFunction, facts); 
    weightSolvingLiterals->set(1, weightFacts);

    std::sort(ownedSolvingLiterals.begin(), ownedSolvingLiterals.end(), 
    [this](const SolvingLiteral* s1, const SolvingLiteral* s2){
        return weightSolvingLiterals->at(s1->id) < weightSolvingLiterals->at(s2->id);
    });

    #ifdef __debug__
        LOG(LogLevel::INFO, "Solving literals of ",pbConstraint," are: ", ownedSolvingLiterals, " - PropagatorPBConstraint::initFirstTime");
    #endif

    for(size_t ni = 0; ni < solver->initializer->nt; ++ni){
        implementations.emplace_back(createImplementation());
    }
}


PropagatorPBConstraintImplementation* PropagatorPBConstraint::createImplementation() noexcept { return new PropagatorPBConstraintImplementationLe(this); }

bool PropagatorPBConstraint::addClausesPropagatedLitIds(const clingo_propagate_control*  control, const std::vector<clingo_literal_t>& propagatedLiteralIds, const clingo_clause_type_t clauseType) noexcept{
    const size_t td = clingo_propagate_control_thread_id((const clingo_propagate_control_t *)control);
    return _addClausesPropagatedLitIds(control, propagatedLiteralIds, false, td, clauseType); 
}

bool PropagatorPBConstraint::_addClausesPropagatedLitIds(const void *control, const std::vector<clingo_literal_t>& propagatedLiteralIds, const bool isInitControl, const size_t td, const clingo_clause_type_t clauseType) noexcept{
    
    const PropagatorPBConstraintImplementation* imp = implementations[td];
    for(size_t si = 0; si < propagatedLiteralIds.size(); ++si){  
        const clingo_literal_t& literalId = propagatedLiteralIds[si];
        const std::vector<clingo_literal_t>& r = imp->getReasonForLiteral(literalId);
        std::vector<clingo_literal_t> clause;
        clause.reserve(r.size() + 1);
        clause.push_back(literalId);
        extendVector(clause, r);
        if(_addClause(control, clause, isInitControl, clauseType))return true;
    }
    return false;
}

void PropagatorPBConstraint::initPropagation(clingo_propagate_init_t *init) noexcept{
    if(isVolatileClauseType()){
        // the clauses are kept for checkImplementation: an init clause would stay after this solve call
        pendingInitClauses.assign(implementations.size(), {});
        bool pending = false;
        for(size_t ni = 0; ni < implementations.size(); ++ni){
            for(const clingo_literal_t literalId: implementations[ni]->initPropagation()){
                std::vector<clingo_literal_t> clause{literalId};
                extendVector(clause, implementations[ni]->getReasonForLiteral(literalId));
                pendingInitClauses[ni].push_back(std::move(clause));
                pending = true;
            }
        }
        clingo_propagate_init_set_check_mode(init, pending ? clingo_propagator_check_mode_fixpoint : clingo_propagator_check_mode_none);
        return;
    }
    for(size_t ni = 0; ni < implementations.size(); ++ni){
        std::vector<clingo_literal_t> derived = implementations[ni]->initPropagation();
        if(derived.size() > 0 && this->addClausesPropagatedLitIds(init, derived, ni)) return;
    }
}

// volatile clause type: adds the clauses of initPropagation of this solve call (once per thread)
bool checkImplementation(clingo_propagate_control_t *control, ClingoPropagator* propagator){
    PropagatorPBConstraint* pbProp = static_cast<PropagatorPBConstraint*>(propagator);
    const size_t td = clingo_propagate_control_thread_id(control);
    if(td >= pbProp->pendingInitClauses.size() || pbProp->pendingInitClauses[td].empty()) return true;
    std::vector<std::vector<clingo_literal_t>> clauses = std::move(pbProp->pendingInitClauses[td]);
    pbProp->pendingInitClauses[td].clear();
    for(const std::vector<clingo_literal_t>& clause: clauses){
        if(pbProp->addClause(control, clause)) return true;     // conflict: this solve call is UNSAT
    }
    return true;
}

bool initImplementation(clingo_propagate_init_t *init, ClingoPropagator* propagator){
    #ifdef __debug__
        auto start = startTimer();
    #endif
    PropagatorPBConstraint* pbProp = static_cast<PropagatorPBConstraint*>(propagator);

    #ifdef __debug__
        LOG(LogLevel::INFO_1, "Called ",pbProp->name," - initPropagator");
    #endif

    // check is registered for every PB propagator: called only when initPropagation asks for it
    clingo_propagate_init_set_check_mode(init, clingo_propagator_check_mode_none);
    if(pbProp->clearIfNotEnabledInInit(init))  return true;

    const bool isFirstTime = pbProp->firstTime;
    if(isFirstTime){  
        pbProp->initFirstTime(init);
        pbProp->firstTime = false;
    }

    pbProp->initPropagation(init);
    if(pbProp->ownedSolvingLiterals.size() == 0) return true;
    
    if(isFirstTime){
        #ifdef __debug__
            LOG(LogLevel::INFO, pbProp," adding watches - initPropagator");
        #endif
        for(const SolvingLiteral* slit: pbProp->ownedSolvingLiterals){
            #ifdef __debug__
                LOG(LogLevel::INFO, pbProp->name," adding watch @ ", slit, " - initPropagator");
            #endif
            const clingo_literal_t &watchedLiteral = slit->id;
            handleError(clingo_propagate_init_add_watch(init, watchedLiteral));
            pbProp->watchedLiterals.push_back(watchedLiteral);
            if(pbProp->watchOppositePolarityLiterals){
                handleError(clingo_propagate_init_add_watch(init, not_(watchedLiteral)));
                pbProp->watchedLiterals.push_back(not_(watchedLiteral));
            }
        }
        #ifdef __debug__
            LOG(LogLevel::INFO, "Adding watch to fact 1");
        #endif
        handleError(clingo_propagate_init_add_watch(init, 1));
        pbProp->watchedLiterals.push_back(1);
    }

    

    #ifdef __debug__
        double elasped = elapsedTime(start);
        propagator->increaseStat("initTime", elasped);
    #endif
    return true;
}

bool propagateImplementation(clingo_propagate_control_t *control, clingo_literal_t const *changes, size_t size, ClingoPropagator* propagator){
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

void undoImplementation(clingo_propagate_control_t const *control, clingo_literal_t const *changes, size_t size, ClingoPropagator* propagator){
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
