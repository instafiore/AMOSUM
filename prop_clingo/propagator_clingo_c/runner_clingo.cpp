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


std::unordered_map<std::string, clingo_literal_t> atomNames ;

void register_propagator(clingo_control_t *ctl, clingo_propagator_t prop, AmoSumPropagator& propagator);

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
    // dummy propagator
    PropagatorClingo dummy_propagator ;
    for(auto& [prop_type, param]: prop_type_params){
        // std::string param_str = unordered_map_to_string(param);
        // printf("%s with param: %s\n", prop_type.c_str(), param_str.c_str());
        register_propagator(ctl, prop, prop_type, param["id"]);
    }
    
    // Loading the program to the control
    if (params.find("enc") != params.end()) handle_error(clingo_control_load(ctl, encoding_path.c_str())) ;
    if (params.find("i") != params.end()) handle_error(clingo_control_load(ctl, instance_path.c_str()));
    

    // ground the pigeon part
    handle_error((clingo_control_ground(ctl, parts, 1, NULL, NULL)));
    
    // solve using a model callback
    handle_error(solve(ctl, &solve_ret));
    

    if (ctl) { clingo_control_free(ctl); }
    
    return ret;

    
}

void register_propagator(clingo_control_t *ctl, clingo_propagator_t prop, std::string prop_type, std::string id){
    // TODO: to implement this:

    // ge, propagate_phase, prop_type = get_propagator_variables(prop_type=prop_type)

    // # Initialize and register the custom propagator
    // param = self.param.copy()
    // param["id"] = id
    // propagator_clingo = PropagatorClingo(param, propagation_phase=propagate_phase, ge=ge, prop_type=prop_type)
    // self.ctl.register_propagator(propagator_clingo)
    // self.propagators.append(propagator_clingo)
  
    //   handle_error(clingo_control_register_propagator(ctl, &prop, &propagator, false));
}
