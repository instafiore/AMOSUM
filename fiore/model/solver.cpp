#include "solver.h"
#include "../utils/logger.h"
#include "../utils/utility.h"
#include "../utils/stats.h"
#include "propagatorGetRest.h"
#include "propagatorGetDerivedFromAss.h"
#include "bound.h"



namespace fiore {

typedef bool (*clingo_propagator_decide_callback_t)(clingo_id_t thread_id, clingo_assignment_t const *assignment, clingo_literal_t fallback, void *propagator, clingo_literal_t *decision);




ClingoSolver::ClingoSolver(std::vector<std::string> ASPInputs)noexcept:initializer(new ClingoPropagatorInitializer()), getterRestLiterals(nullptr), getterDerivedFromAss(nullptr), changedPolTargets(true), collectUnsatCores(true) {
    Program::ProgramGrounder grounder;
    program.reset(grounder.ground(ASPInputs));
    #ifdef __debug__
        LOG(LogLevel::DEBUG, "Created program:", program);
    #endif
    // unweighted.reset(new UMNWeightFunction(this->program->maxSoftLiteralId + 1, 1));
    this->enableRestDetection();
    for(const Literal* t: *program->softLiterals) coreLowerBound.addTarget(t->id, program->weightFunction->at(t->id));
    if(ArgParse::getInstance()->get(CONSTANTS::enableAlreadyDerivedFromAssumptionDetectionKey) == CONSTANTS::TRUE_STR) this->enableGetDerivedFromAssDetection();
}

ClingoSolver::~ClingoSolver(){
    for(Core* c: unsatCores) delete c;
}

void ClingoSolver::setTORC(const Assignment* a, size_t priority) noexcept { 
    this->setPolarity0(priority); // Target Optimistic 
    this->setRest(a, priority); // Rest Conservative
}

void ClingoSolver::setRest(const Assignment* a, size_t priority) noexcept { 
    if(getterRestLiterals->rest == nullptr) return;
    this->program->setPolaritiesLocalSearch(a, priority, getterRestLiterals->rest); // Rest Conservative
}


void ClingoSolver::registerPropagator(ClingoPropagator* propagator, std::string name){
    clingo_propagator_t prop_struct = {
        propagator->init != nullptr ? (clingo_propagator_init_callback_t)propagator->init : NULL,
        propagator->propagate != nullptr ? (clingo_propagator_propagate_callback_t)propagator->propagate : NULL,
        propagator->undo != nullptr ? (clingo_propagator_undo_callback_t)propagator->undo : NULL,
        propagator->check != nullptr ? (clingo_propagator_check_callback_t)propagator->check : NULL,
        propagator->decide != nullptr ? (clingo_propagator_decide_callback_t)propagator->decide : NULL,
    };
    handleError(clingo_control_register_propagator(program->ctl, &prop_struct, propagator, false));
    #ifdef __debug__
        const size_t &n = propagators.size();
        std::string propStr = propagator->decide != nullptr ? "Decider " : "";
        LOG(LogLevel::INFO_1, "Registering ",propStr," P(",n,"): ",propagator);
    #endif
    propagators.emplace_back(propagator);
 }


bool solveCallback(clingo_solve_event_type_t type, void *event, void *data, bool *goon){
    #ifdef __debug__
        LOG(LogLevel::DEBUG, "clingo_solve_event_type_t type: ",type);
        auto startTime = startTimer();
    #endif
    
    
    ClingoSolver* solver = (ClingoSolver*)data;

    // clingo_solve_event_type_model = 0,      //!< Issued if a model is found.
    // clingo_solve_event_type_unsat = 1,      //!< Issued if an optimization problem is found unsatisfiable.
    // clingo_solve_event_type_statistics = 2, //!< Issued when the statistics can be updated.
    // clingo_solve_event_type_finish = 3,
    
    if (type == clingo_solve_event_type_model) {
        // event is a pointer to a clingo_model_t const *
        clingo_model_t const *model = (clingo_model_t const *)event;

        if (model != NULL) {
            // const size_t n = solver->answerSets.size();
            #ifdef __debug__
                LOG(LogLevel::DEBUG, "solveCallback: SAT");
            #endif
            solver->answerset = new ClingoAssignment(solver->program.get(), model);
            AnswersetsPool::getInstance()->addAnswerset(solver->answerset);
            // solver->answerSets.emplace_back(answeset);
            // Program* program, const size_t number, clingo_model_t* model
        }
    }
    else if (type == clingo_solve_event_type_unsat) {
        // Ignore
    }
    else if (type == clingo_solve_event_type_statistics) {
        // clingo_statistics_t const **stats = (clingo_statistics_t const **)event;
        // clingo_statistics_t const *step     = stats[0];
        // clingo_statistics_t const *accum    = stats[1];

        // // step = statistics for this step,
        // // accum = accumulated statistics
    }
    else {
        clingo_solve_result_bitset_t *result = (clingo_solve_result_bitset_t *)event;
        solver->res = clingo::ClingoResult(*result);
    }

    #ifdef __debug__
        auto elapsedTimeCallbackSolve = elapsedTime(startTime);
        LOG(LogLevel::DEBUG,"elapsedTimeCallbackSolve: ",elapsedTimeCallbackSolve);
    #endif
    return true;

    *goon = true;
    return true;
}


void ClingoSolver::setConflictLimit(std::string max_conflicts) {
    this->setConfiguration("solve/solve_limit",max_conflicts);
}


std::pair<ClingoAssignment*, const clingo::ClingoResult> ClingoSolver::solve(const std::vector<clingo_literal_t>& assumptionsOnTheFly) noexcept{


    // TODO: To remove and optimize it
    // if(assumptionsOnTheFly != nullptr) for(const clingo_literal_t& l: *assumptionsOnTheFly) addAssumption(l);

    clingo_solve_handle_t *handle = NULL;

    bool result;

    #ifdef __debug__
        auto startTimeSolve = startTimer();
    #endif
    this->answerset = nullptr;
    std::vector<clingo_literal_t> assumptionsCurrentCall = assumptions;
    extendVector(assumptionsCurrentCall, assumptionsOnTheFly);

    #ifdef __debug__
        LOG(LogLevel::INFO,"Solving under assumptions: ",this->literalIdsToLiterals(assumptionsCurrentCall));
    #endif
    handleError(clingo_control_solve(program->ctl, clingo_solve_mode_yield, assumptionsCurrentCall.data(), assumptionsCurrentCall.size(), &solveCallback, this, &handle));
    #ifdef __debug__
        auto elaspedTimeSolve = elapsedTime(startTimeSolve);
    #endif
    clingo_solve_handle_wait(handle,-1,&result);

    if(!this->res.isSat()){
        blamedAssumptions.clear();
        if (this->res.isUnsat()) {
            Core* core = new Core();
            clingo_literal_t const *corePointer = nullptr;
            size_t coreSize = 0;
            handleError(clingo_solve_handle_core(handle, &corePointer, &coreSize));
            for(size_t i=0; i<coreSize; ++i){ 
                const clingo_literal_t& assumption = corePointer[i];
                core->add(assumption);
                blamedAssumptions.add(assumption);
            }
            if(collectUnsatCores){
                unsatCores.push_back(core);
                if(coreLowerBound.addCore(*core)){
                    #ifndef NSTATS
                        Statistics::getInstance()->addSeriesPoint("lowerBound", getLowerBound());
                    #endif
                }
            }else delete core;
        }else if(this->res.isUnkown()){
            for(const clingo_literal_t& l: this->assumptions) blamedAssumptions.add(l);
        }
    }

    clingo_solve_handle_close(handle);


    auto pairAssignmentRes = this->res.isSat()
        ? std::make_pair(this->answerset, std::ref(this->res))
        : std::make_pair(nullptr, std::ref(this->res));
    
    ;


    this->updateStatistics();
    const Statistics* stats = Statistics::getInstance();
    const double& elapsed = stats->statsLastSolve.at("time");
    if(this->res.isSat()) pairAssignmentRes.first->setTime(elapsed);


    ++countSolve;
    #ifdef __debug__
        if(countSolve == 1) firstCallTime = elaspedTimeSolve;
        totalTimeSolve += elapsed;
        totalTimeSolveReal += elaspedTimeSolve;
        const double tmpTotal = totalTimeSolveReal - firstCallTime;
        std::string costString = this->res.isSat() ? " Cost: "+std::to_string(this->answerset->cost()) : "";
        if(elapsed >= 0.001 || true){
            std::string stringStatisticsSolver = "";
            #ifndef NSTATS
                std::ostringstream oss;
                oss<< "Statistics Solver: "<<stats;
                stringStatisticsSolver = oss.str();
            #endif
            // LOG(LogLevel::INFO_2,"Finished! core: ",getCore()," totalTimeSolveReal: ",tmpTotal," totalTimeSolve: ",totalTimeSolve," Solving Time real: ",elaspedTimeSolve," Solving time: ", elapsed, "s", " countSolve: ",countSolve, " Result: ", this->res, costString, " assumptions size: ", assumptionsCurrentCall.size(), " ",stringStatisticsSolver," - ClingoSolver::solve");
            LOG(LogLevel::DEBUG_3,"Finished! totalTimeSolveReal: ",tmpTotal," totalTimeSolve: ",totalTimeSolve," Solving Time real: ",elaspedTimeSolve," Solving time: ", elapsed, "s", " countSolve: ",countSolve, " Result: ", this->res, costString, " assumptions size: ", assumptionsCurrentCall.size(), " ",stringStatisticsSolver," - ClingoSolver::solve");
            // LOG(LogLevel::DEBUG,"Answer Set: ",this->lastCallAnswerSetToString());
        }
        this->restartWallTimePropagators();
        // const clingo_statistics_t *statsClingo;
        // handleError(clingo_control_statistics(this->program->ctl, &statsClingo));
        // uint64_t key;
        // handleError(clingo_statistics_root(statsClingo, &key));
        // printClingoStatistics(statsClingo, key);
        // exit(0);
    #endif

    return pairAssignmentRes;
 } 





bool ClingoSolver::atLeast2Models(const std::vector<clingo_literal_t>* assumptions){
    #ifdef __debug__
        const std::vector<clingo_literal_t> assIds = assumptions != nullptr ? *assumptions : std::vector<clingo_literal_t>();
        LOG(LogLevel::INFO,"Checking at least 2 models with assumptions: ",this->literalIdsToLiterals(assIds));
    #endif

    size_t countModels = 0;
    this->setConfiguration("solve/models","2");
    clingo_solve_handle_t *handle;
    clingo_model_t const *model;
 
    handleError(clingo_control_solve(program->ctl, clingo_solve_mode_yield, assumptions != nullptr ? assumptions->data() : NULL , assumptions != nullptr ? assumptions->size() : 0, NULL, NULL, &handle));
    while (true) {
        handleError(clingo_solve_handle_resume(handle));
        handleError(clingo_solve_handle_model(handle, &model));
        if (model) { ++countModels; }
        else { break; }
    }
    clingo_solve_result_bitset_t result;
    handleError(clingo_solve_handle_get(handle, &result));
    
    clingo_solve_handle_close(handle);
    this->setConfiguration("solve/models","-1");

    return countModels >= 2;
}


void ClingoSolver::enableRestDetection(){
    if(getterRestLiterals == nullptr){
        getterRestLiterals = new PropagatorGetRestLiterals("Getter Facts",this);
        registerPropagator(getterRestLiterals);
        #ifdef __debug__
            LOG(LogLevel::INFO_1, "Enabled Getter Facts");
        #endif
    }
    getterRestLiterals->enable();
}

void ClingoSolver::disableRestDetection(){
    if(getterRestLiterals != nullptr) getterRestLiterals->disable();
}


void ClingoSolver::enableGetDerivedFromAssDetection(){
    if(getterDerivedFromAss == nullptr){
        getterDerivedFromAss = new PropagatorGetDerivedFromAss("Getter Derived From Ass",this);
        #ifdef __debug__
            LOG(LogLevel::INFO_1, "Enabled Getter Derived From Ass");
        #endif
    }
    registerPropagator(getterDerivedFromAss);
    getterDerivedFromAss->enable();
}

void ClingoSolver::disableGetDerivedFromAssDetection(){
    if(getterDerivedFromAss != nullptr) getterDerivedFromAss->disable();
}


void ClingoSolver::addAssumption(const clingo_literal_t &l) noexcept { 
    if(this->getterDerivedFromAss != nullptr) 
        this->getterDerivedFromAss->addAssumption(l); 
    assumptions.push_back(l);
    assumptionsSet.emplace(l);
}

void ClingoSolver::addAssumptionAndSet(const clingo_literal_t &l, std::vector<clingo_literal_t>&& derivedFromL) noexcept {
    if(this->getterDerivedFromAss != nullptr) this->getterDerivedFromAss->addAssumptionAndSet(l, std::move(derivedFromL));
    assumptions.push_back(l);
    assumptionsSet.emplace(l);
}

void ClingoSolver::clearAssumptions() noexcept { if(this->getterDerivedFromAss != nullptr) this->getterDerivedFromAss->clearAssumptions(); assumptions.clear(); assumptionsSet.clear();}




bool ClingoSolver::isDerivedFromCurrentAssumptions(const clingo_literal_t& plit) noexcept { 
    if(this->getterDerivedFromAss != nullptr) 
        return this->getterDerivedFromAss->isDerivedFromAssumptions(plit);
    else return false;
}

void ClingoSolver::removeFromIToLastAssumption(const size_t& i) noexcept { 
    if(i>=assumptions.size()) return; 
    if(this->getterDerivedFromAss != nullptr) this->getterDerivedFromAss->removeFromIToLastAssumption(i); 
    for(size_t k=i; k < assumptions.size(); ++k){
        assumptionsSet.erase(assumptions[k]);
    }
    assumptions.resize(i); 
}

std::pair<std::vector<clingo_literal_t>, std::unordered_map<clingo_literal_t, std::vector<clingo_literal_t>>> ClingoSolver::removeFromIToLastAssumptionAndReturn(const size_t i) noexcept{
    std::vector<clingo_literal_t> removed;
    std::unordered_map<clingo_literal_t, std::vector<clingo_literal_t>> mapImplicationRemovedAssumptions;
    for(size_t k=i; k < assumptions.size(); ++k){
        mapImplicationRemovedAssumptions[assumptions[k]] = this->derivedLiteralsFromAssumption(assumptions[k]);
        removed.push_back(assumptions[k]);
    }
    removeFromIToLastAssumption(i);
    return std::make_pair<>(removed, mapImplicationRemovedAssumptions) ;
}


const std::vector<clingo_literal_t>& ClingoSolver::derivedLiteralsFromAssumptionIndex(const size_t& i) noexcept{
    if(this->getterDerivedFromAss == nullptr || i>=assumptions.size()) return clingo::EMPTY_LITERALS;
    const clingo_literal_t assumption = assumptions[i];
    return this->getterDerivedFromAss->derivedFromAssumption(assumption);
}

const std::vector<clingo_literal_t>& ClingoSolver::derivedLiteralsFromAssumption(const clingo_literal_t& assumption) noexcept{
    return this->getterDerivedFromAss->derivedFromAssumption(assumption);
}

int64_t ClingoSolver::getCostFromFacts(const UMNWeightFunction* weightFunction) noexcept{
    weightFunction = weightFunction == nullptr ? this->program->weightFunction.get() : weightFunction;
    int64_t cost =  0;
    if(this->getterDerivedFromAss == nullptr){
        for(const Literal* l: this->initializer->factsVectorProgramLiterals){
            cost += weightFunction->at(l->id);
        }
    }else{
        for(const clingo_literal_t& plit: this->getterDerivedFromAss->derivedFromAssumption(CONSTANTS::TOP)){
            cost += weightFunction->at(plit);
        }
    }
    return cost;
}



void ClingoSolver::removeLastAssumption() noexcept { if(assumptions.empty()) return; if(this->getterDerivedFromAss != nullptr) this->getterDerivedFromAss->removeLastAssumption(); assumptionsSet.erase(assumptions.back()); assumptions.pop_back(); }




std::ostream& operator<<(std::ostream& out, const Core& core){
    out<<core.core;
    return out;
}

void ClingoSolver::updateStatistics() const noexcept{
    Statistics* s = Statistics::getInstance();
    const clingo_statistics_t *stats;
    handleError(clingo_control_statistics(program->ctl, &stats));

    s->statsLastSolve["conflicts"] = clingo::getValueStatisticClingo(stats, "solving/solvers/conflicts");
    s->statsLastSolve["choices"] = clingo::getValueStatisticClingo(stats, "solving/solvers/choices");
    s->statsLastSolve["conflicts_analyzed"] = clingo::getValueStatisticClingo(stats, "solving/solvers/conflicts_analyzed");
    s->statsLastSolve["restarts"] = clingo::getValueStatisticClingo(stats, "solving/solvers/restarts");
    s->statsLastSolve["restarts_last"] = clingo::getValueStatisticClingo(stats, "solving/solvers/restarts_last");
    s->statsLastSolve["restarts_blocked"] = clingo::getValueStatisticClingo(stats, "solving/solvers/restarts_blocked");
    s->statsLastSolve["lits_conflict"] = clingo::getValueStatisticClingo(stats, "solving/solvers/extra/lits_conflict");
    #ifdef __debug__
        for(size_t i = 0; i < propagators.size(); ++i) s->customStatsSolver[" P("+ std::to_string(i)+ ")"] = propagators[i]->getStatistics();
    #endif
    s->statsLastSolve["time"] = clingo::getValueStatisticClingo(stats, "summary/times/solve");
    s->statsSolver["total_time"] = clingo::getValueStatisticClingo(stats, "accu/times/total");
}

int64_t ClingoSolver::getCoreLowerBound() const noexcept{ return coreLowerBound.value(); }
void ClingoSolver::addForcedToLowerBound(const clingo_literal_t& l) noexcept{ 
    if(coreLowerBound.addForced(l)){
        #ifndef NSTATS
            Statistics::getInstance()->addSeriesPoint("lowerBound", program->preprocessedCost + coreLowerBound.value());
        #endif
    }
}

} // namespace fiore
