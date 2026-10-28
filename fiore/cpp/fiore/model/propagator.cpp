#include "propagator.h"
#include "../utils/logger.h"
#include "../utils/argparse.h"
#include "solver.h"
// inline ClingoPropagatorInitializer* ClingoPropagatorInitializer::instance = nullptr;



namespace fiore {

std::ostream& operator<<(std::ostream& out, const ClingoPropagator& p){
    return out<<p.getName();
}


void ClingoPropagatorInitializer::init(clingo_propagate_init* _init, ClingoSolver* solver) noexcept{
    
    if(!first) return; 
    first = false;
    nt = clingo_propagate_init_number_of_threads(_init);

    MapLiteralId2Literal* mapLiteralId2Literal = solver->program->mapLiteralId2Literal.get();
    std::unordered_map<clingo_literal_t, std::pair<bool,std::unordered_set<const Literal*>>> mapSlitIdPlitsTmp;

    #ifdef __debug__
        LOG(LogLevel::DEBUG, "number of threads: ",nt, " - ClingoPropagatorInitializer::init");
    #endif

    const Vector<const Literal*> &restLiterals = *solver->program->restLiterals.get();
    const Vector<const Literal*> &softLiterals = *solver->program->softLiterals.get();
    maxSoftSolverLiteralId = 0;
    for(const Literal* literal: softLiterals){
        const clingo_literal_t &plitId = literal->id;
        clingo_literal_t slitId;
        clingo_propagate_init_solver_literal(_init, plitId, &slitId);   
        this->mapSoftPlit2SoftSlit[plitId] = slitId;
        this->mapSoftPlit2SoftSlit[not_(plitId)] = not_(slitId);
        clingo_atom_t atomId = std::abs(slitId);
        if(maxSoftSolverLiteralId < atomId) maxSoftSolverLiteralId = atomId;
        setDefault(mapSlitIdPlitsTmp, slitId, std::make_pair<>(true,std::unordered_set<const Literal*>()));
        mapSlitIdPlitsTmp[slitId].first = true;
        mapSlitIdPlitsTmp[slitId].second.insert(literal);
        const Literal* negatedLiteral = mapLiteralId2Literal->literal(not_(literal->id));
        setDefault(mapSlitIdPlitsTmp, not_(slitId), std::make_pair<>(false,std::unordered_set<const Literal*>()));
        mapSlitIdPlitsTmp[not_(slitId)].second.insert(negatedLiteral);
    }
    this->mapSoftPlit2SoftSlit[CONSTANTS::TOP] = CONSTANTS::TOP;
    this->mapSoftPlit2SoftSlit[CONSTANTS::BOTTOM] = CONSTANTS::BOTTOM;
    auto cmd = ArgParse::getInstance();
    const bool torc = cmd->get(CONSTANTS::polarityHeuristic) == CONSTANTS::polarityHeuristicTORC;

    #ifdef __debug__
        auto startTimeRestLiterals = startTimer();
    #endif

    if(torc){
        for(const Literal* literal: restLiterals){
            const Literal* negatedLiteral = mapLiteralId2Literal->literal(not_(literal->id));
            const clingo_literal_t &plitId = literal->id;
            clingo_literal_t slitId;
            clingo_propagate_init_solver_literal(_init, plitId, &slitId);   
            clingo_atom_t atomId = std::abs(slitId);
            // setDefault(mapSlitIdPlitsTmp, slitId, std::make_pair<>(true,std::unordered_set<const Literal*>()));
            // mapSlitIdPlitsTmp[slitId].first = true;
            // mapSlitIdPlitsTmp[slitId].second.insert(literal);
            // setDefault(mapSlitIdPlitsTmp, not_(slitId), std::make_pair<>(false,std::unordered_set<const Literal*>()));
            // mapSlitIdPlitsTmp[not_(slitId)].second.insert(negatedLiteral);
            // if(maxSolverLiteralId < atomId) maxSolverLiteralId = atomId;
            if(atomId != 1) programAtomRestNoFact.push_back(literal->id > 0 ? literal : negatedLiteral);
        }
    }

    #ifdef __debug__
        auto elaspedTimeRestLiterals = elapsedTime(startTimeRestLiterals);
        LOG(LogLevel::DEBUG, "Time for creating rest literals: ",elaspedTimeRestLiterals);
    #endif

    mapSoftSlitId2SoftSlit.reset(new MapLiteralId2Literal(maxSoftSolverLiteralId + 1));

    for(const auto&[slitId, plitsWithFlag]: mapSlitIdPlitsTmp){
        const bool& isInsidePb = plitsWithFlag.first;
        const std::unordered_set<const Literal*>& plits = plitsWithFlag.second;
        const SolvingLiteral* solvingLiteral = new SolvingLiteral(slitId, plits);
        mapSoftSlitId2SoftSlit->set(slitId, solvingLiteral);
        clingo_atom_t atomId = std::abs(slitId);
        if(!isInsidePb) continue;
        globalSoftSolvingLiterals.push_back(solvingLiteral);
        if(atomId == 1){
            for(const Literal* l: plits){ 
                factsVectorProgramLiterals.push_back(l);
                factsSetProgramLiterals.emplace(l->id);
            }
        }
    }

    if(mapSlitIdPlitsTmp.find(CONSTANTS::TOP) == mapSlitIdPlitsTmp.end()){
        const TOP* solvingLiteralTop = new TOP();
        const BOTTOM* solvingLiteralBottom = new BOTTOM();
        mapSoftSlitId2SoftSlit->set(solvingLiteralTop->id, solvingLiteralTop);
        mapSoftSlitId2SoftSlit->set(solvingLiteralBottom->id, solvingLiteralBottom);
    }

    #ifdef __debug__
        LOG(LogLevel::INFO, "soft literals initializer ",softLiterals, " - ClingoPropagatorInitializer::init");
        LOG(LogLevel::DEBUG, "rest literals initializer ",restLiterals, " - ClingoPropagatorInitializer::init");
        LOG(LogLevel::DEBUG_1, "Global softSolvingLiterals:: ",globalSoftSolvingLiterals, " - ClingoPropagatorInitializer::init");
    #endif
}   


bool ClingoPropagator::_addClause(const void *control, const std::vector<clingo_literal_t>& clause, const bool isInitControl, const clingo_clause_type_t clauseType) noexcept {
    bool result_add_clause;
    
    clingo_propagate_init* controlInit = isInitControl ? (clingo_propagate_init*) control: nullptr;
    clingo_propagate_control* controlPropagate = !isInitControl ? (clingo_propagate_control*) control: nullptr;

    if(isInitControl) handleError(clingo_propagate_init_add_clause(controlInit, clause.data(), clause.size(), &result_add_clause));
    else handleError(clingo_propagate_control_add_clause(controlPropagate, clause.data(), clause.size(), clauseType != CONSTANTS::NONE32 ? clauseType : clauseTypeGlobal, &result_add_clause)) ;
    #ifdef __debug__
        LOG(LogLevel::DEBUG_1,"Adding Clause [slits]: ", this->solvingLiteralIdsToSolvingLiterals(clause));
    #endif

    // propagation must return immediately, there is a conflict
    if (!result_add_clause){
        #ifdef __debug__
            LOG(LogLevel::DEBUG_1,"Conflict with clause [slits]: ", this->solvingLiteralIdsToSolvingLiterals(clause));
            if(!isInitControl){
                LOG(LogLevel::DEBUG_1, this->solvingLiteralIdToSolvingLiteral(clause[0])," is undef: ",fiore::clingo::isUndef(clingo_propagate_control_assignment(controlPropagate), clause[0]), " clause[0]: ",clause[0]);
                LOG(LogLevel::DEBUG_1, this->solvingLiteralIdToSolvingLiteral(clause[0])," is true: ",fiore::clingo::isTrue(clingo_propagate_control_assignment(controlPropagate), clause[0]));
                LOG(LogLevel::DEBUG_1, this->solvingLiteralIdToSolvingLiteral(clause[0])," is false: ",fiore::clingo::isFalse(clingo_propagate_control_assignment(controlPropagate), clause[0]));
            }
        #endif
        return true ;
    }
    if(isInitControl) return false;
     
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

bool ClingoPropagator::clearIfNotEnabledInInit(clingo_propagate_init* init){
    if(!this->isEnabled()){
        #ifdef __debug__
            LOG(LogLevel::DEBUG_1, "Propagator: ",name, " is not enabled");
        #endif
        removeWatchAll(init);
        return true;
    }
    return false;
}

std::vector<const Literal*> ClingoPropagator::solvingLiteralIdsToSolvingLiterals(clingo_literal_t const *changes, size_t size) noexcept{
    std::vector<const Literal*> res;
    MapLiteralId2Literal* mapLitId2Lit = this->solver->initializer->mapSoftSlitId2SoftSlit.get();
    for(size_t i = 0; i < size; ++i) res.push_back(mapLitId2Lit->get(changes[i]));
    return res;
}

std::vector<const Literal*> ClingoPropagator::solvingLiteralIdsToSolvingLiterals(const std::unordered_set<clingo_literal_t>& changes) noexcept{
    std::vector<const Literal*> res;
    MapLiteralId2Literal* mapLitId2Lit = this->solver->initializer->mapSoftSlitId2SoftSlit.get();
    for(const clingo_literal_t& lit: changes) res.push_back(mapLitId2Lit->get(lit));
    return res;
}


void ClingoPropagator::removeWatchAll(clingo_propagate_init* init){
    for(clingo_literal_t watched: watchedLiterals){
        #ifdef __debug__
            LOG(LogLevel::DEBUG,"Removing watch from: ",watched);
        #endif  
        handleError(clingo_propagate_init_remove_watch(init, watched));
        
    }
    watchedLiterals.clear();
}

void ClingoPropagator::initFirstTime(clingo_propagate_init* _init){

    #ifdef __debug__
        LOG(LogLevel::INFO_1, "Soft literals for propagator ",name," : ",programLiterals(), " - ClingoPropagator::initFirstTime");
    #endif

    solver->initializer->init(_init, this->solver);
    const Vector<const Literal*>&  plits = programLiterals();
    this->ownedSolvingLiterals.clear();
    for(const SolvingLiteral* solvingLiteral: solver->initializer->globalSoftSolvingLiterals){
        const std::unordered_set<const Literal*>& programLiteralsSlit = solvingLiteral->programLiterals;
        bool anyInside = false;
        for(const Literal* plit: plits){

            auto it = std::find(programLiteralsSlit.begin(), programLiteralsSlit.end(), plit);
            if(it != programLiteralsSlit.end()){
                anyInside = true;
                break;
            }
        }
        if(anyInside || solvingLiteral->id == CONSTANTS::TOP) this->ownedSolvingLiterals.push_back(solvingLiteral);
    }
}

} // namespace fiore
