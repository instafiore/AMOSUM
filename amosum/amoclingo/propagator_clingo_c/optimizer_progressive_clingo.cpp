
#include "optimizer_progressive_clingo.h"

bool OptimizerProgressiveClingo::init(clingo_propagate_init_t *_init){
    clingo_propagate_init_set_check_mode(_init, clingo_propagator_check_mode_total);
    propagator->bound = 0;
    return true;
}

OptimizerProgressiveClingo* OptimizerProgressiveClingo::getInstance(){
    // if(instance == nullptr) assert(false);
    return instance;
}

void OptimizerProgressiveClingo::initInstace(const ParameterMap &params,PropagatorClingo* propagator){
    instance = new OptimizerProgressiveClingo(params, propagator);
}
 
bool OptimizerProgressiveClingo::check(clingo_propagate_control_t *control){
    const clingo_assignment_t *assignment = clingo_propagate_control_assignment(control);
    int dl = clingo_assignment_decision_level(assignment);
    int td; 
    dl == 0 ? td = 0 : td = clingo_propagate_control_thread_id(control);
    if(currentAnswerSet != nullptr) delete currentAnswerSet;
    currentAnswerSet = new AnswerSet(new Model(assignment));
    if(params.find("serialize") == params.end())  printf("%s\n",currentAnswerSet->toString().c_str());
    else  printf("%s\n",currentAnswerSet->serialize().c_str());
    int mps = propagator->propagators[td]->_mps;
    propagator->updateBound(mps + 1, td);
    propagator->propagators[td]->mps_violated = true;
    const std::vector<clingo_literal_t>* propagated = propagator->propagation_phase(nullptr, propagator->propagators[td]);
    if(propagated != nullptr) propagator->add_clauses_propagated_lits(control, *propagated, dl, false);
    return true;
}

void OptimizerProgressiveClingo::discardCurrentCost(clingo_propagate_control_t *control, size_t td){

    // std::vector<clingo_literal_t> clause;
    // for (auto* g : propagator->propagators[td]->groups) {
    //     if (propagator->propagators[td]->true_group->get(g) != SETTINGS::NONE) {
    //         clingo_literal_t programLiteral = not_(propagator->propagators[td]->true_group->get(g));
    //         clingo_literal_t solvingLiteral = *(propagator->propagators[td]->map_plit_slit)[programLiteral];
    //         clause.push_back(solvingLiteral);
    //     }
    // }

    // bool result_add_clause;
    // handle_error(clingo_propagate_control_add_clause((clingo_propagate_control*) control, clause.data(), clause.size(), clingo_clause_type_learnt, &result_add_clause));

    // if (not result_add_clause){
    //     return;
    // }

    // bool result_propagate;
    // handle_error(clingo_propagate_control_propagate((clingo_propagate_control*)control, &result_propagate));
}

OptimizerProgressiveClingo* OptimizerProgressiveClingo::instance = nullptr ;