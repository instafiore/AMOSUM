
#include "optimizer_clingo.h"

bool OptimizerClingo::init(clingo_propagate_init_t *_init){
    clingo_propagate_init_set_check_mode(_init, clingo_propagator_check_mode_total);
    propagator->bound = 0;
    return true;
}

OptimizerClingo* OptimizerClingo::getInstance(){
    if(instance == nullptr) assert(false);
    return instance;
}

void OptimizerClingo::initInstace(const ParameterMap &params,PropagatorClingo* propagator){
    instance = new OptimizerClingo(params, propagator);
}
 
bool OptimizerClingo::check(clingo_propagate_control_t *control){
    const clingo_assignment_t *assignment = clingo_propagate_control_assignment(control);
    int dl = clingo_assignment_decision_level(assignment);
    int td; 
    dl == 0 ? td = 0 : td = clingo_propagate_control_thread_id(control);
    if(currentAnswerSet != nullptr) delete currentAnswerSet;
    // currentAnswerSet = new AnswerSet(*propagator->atomNames, &propagator->weights_names(), assignment, propagator->map_plit_slit);
    currentAnswerSet = new AnswerSet(new Model(*propagator->atomNames, &propagator->weights_names(), assignment, propagator->map_plit_slit));
    if(params.find("serialize") == params.end())  printf("%s\n",currentAnswerSet->toString().c_str());
    else  printf("%s\n",currentAnswerSet->serialize().c_str());
    int mps = propagator->propagators[td]->_mps;
    propagator->updateBound(mps + 1, td);
    propagator->propagators[td]->mps_violated = true;
    const std::vector<clingo_literal_t>* propagated = propagator->propagation_phase(nullptr, propagator->propagators[td]);
    if(propagated != nullptr) propagator->add_clauses_propagated_lits(control, *propagated, dl, false);
    return true;
}

OptimizerClingo* OptimizerClingo::instance = nullptr ;