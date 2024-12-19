#include <clingo.h>
#include <stdlib.h>
#include <stdio.h>
#include <string>
#include <assert.h>
#include "../../utility.h"
#include "../../amosum.h"
#include <sstream>
#include <iostream>
#include <vector>
#include <limits>
#include "propagator_clingo.h"


void register_propagator(clingo_control_t *ctl, clingo_propagator_t prop, std::string prop_type, ParameterMap param, std::vector<PropagatorClingo*> &propagators);

bool init(clingo_propagate_init_t *init, PropagatorClingo *propagator){
    return propagator->init(init);
}

bool propagate(clingo_propagate_control_t *control, const clingo_literal_t *changes, size_t size, AmoSumPropagator *propagator){
    return true;
}

void undo(clingo_propagate_control_t *control, const clingo_literal_t *changes, size_t size, AmoSumPropagator *propagator){

}

int main(int argc, char const *argv[])
{
    debugf("propagator_clingo.cpp started");

    std::unordered_map<std::string, std::string> params =  init_param(argc, argv);
    // print_unordered_map(params);

    std::string encoding_path = "" ;
    params.find("enc") != params.end() ? encoding_path = params.find("enc")->second : NULL ;
    std::string instance_path = "" ;
    params.find("i") != params.end() ? instance_path = params.find("i")->second : NULL ;

    // printf("encoding: %s\n", encoding_path.c_str());
    debugf("encoding: ", encoding_path.c_str());
    debugf("instance: ", instance_path.c_str());

    std::string encoding = cat(encoding_path);
    std::string instance = cat(instance_path);

    ParameterMap::iterator it_amosum = params.find("amosum_propagator")  ;

    std::vector<std::string> sys_parameters;
    if (it_amosum != params.end()){
        std::string sys_parameters_str = it_amosum->second ;
        sys_parameters = split(sys_parameters_str, ' ');
    }

    char const *error_message;
    int ret = 0;
    clingo_solve_result_bitset_t solve_ret;
    clingo_control_t *ctl = NULL;
    handle_error(clingo_control_new(NULL, 0, NULL, NULL, 20, &ctl));
    clingo_part_t parts[] = {{"base", NULL, 0 }};

    // create a propagator with the functions above
    // using the default implementation for the model check
    clingo_propagator_t prop = {
        (clingo_propagator_init_callback_t)init,
        (clingo_propagator_propagate_callback_t)propagate,
        (clingo_propagator_undo_callback_t)undo,
        NULL,
        NULL,
    };


    std::vector<std::pair<std::string, ParameterMap>> prop_type_params = process_sys_parameters(sys_parameters) ;

    std::vector<PropagatorClingo*> propagators ;
    for(auto& [prop_type, param]: prop_type_params){
        register_propagator(ctl, prop, prop_type, param, propagators);
    }
    
    // Loading the program to the control
    if (params.find("enc") != params.end()) handle_error(clingo_control_load(ctl, encoding_path.c_str())) ;
    if (params.find("i") != params.end()) handle_error(clingo_control_load(ctl, instance_path.c_str()));
    

    // ground the pigeon part
    handle_error((clingo_control_ground(ctl, parts, 1, NULL, NULL)));
    
    // solve using a model callback
    handle_error(solve(ctl, &solve_ret));
    

    // FREE
    for(auto& propagator: propagators){
        delete propagator;
    }
    if (ctl) { clingo_control_free(ctl); }
    
    return ret;

    
}

void register_propagator(clingo_control_t *ctl, clingo_propagator_t prop, 
    std::string prop_type, ParameterMap param,
    std::vector<PropagatorClingo*> &propagators){

    std::tuple<bool, 
    const std::vector<clingo_literal_t>* (*)(const Group &G, 
    AmoSumPropagator &propagator), std::string> result = get_propagator_variables(prop_type=prop_type);

    bool ge = std::get<0>(result);
    const std::vector<clingo_literal_t>* (*propagation_phase)(const Group &G, AmoSumPropagator &propagator) = std::get<1>(result);
    std::string choice_cons = std::get<2>(result);

    PropagatorClingo* propagator_clingo = new PropagatorClingo(param, propagation_phase, ge, choice_cons);

    handle_error(clingo_control_register_propagator(ctl, &prop, propagator_clingo, false));
    propagators.push_back(propagator_clingo);
}
