#include <clingo.h>
#include <stdlib.h>
#include <stdio.h>
#include <string>
#include <assert.h>
#include "../../utility.h"
#include "../../amosum.h"
#include <sstream>
#include <iostream>

void register_propagator(clingo_control_t *ctl, clingo_propagator_t prop, AmoSumPropagator& propagator);

bool init(clingo_propagate_init_t *init, AmoSumPropagator *propagator){
    debug("[init] entered")
    size_t threads = clingo_propagate_init_number_of_threads(init);

    // std::vector<std::tuple<std::string, int, int>> atoms_list_for_mapping;

    clingo_literal_t max_plit = 0;
    clingo_symbolic_atoms_t const *symbolic_atoms;
    clingo_symbolic_atom_iterator_t symbolic_atoms_it, symbolic_atoms_ie;
    
    handle_error(clingo_propagate_init_symbolic_atoms(init, &symbolic_atoms));
    handle_error(clingo_symbolic_atoms_end(symbolic_atoms, &symbolic_atoms_ie));

    handle_error(clingo_symbolic_atoms_begin(symbolic_atoms, NULL, &symbolic_atoms_it));

    int cont = 0 ;
    while (true) {
        bool equal;
        clingo_literal_t plit;
        clingo_literal_t slit;
        clingo_symbol_t symbol;
 
        // stop iteration if the end is reached
        handle_error(clingo_symbolic_atoms_iterator_is_equal_to(symbolic_atoms, symbolic_atoms_it, symbolic_atoms_ie, &equal));
        if (equal) { break; }
        handle_error(clingo_symbolic_atoms_symbol(symbolic_atoms, symbolic_atoms_it, &symbol));
        char symbol_str_c[256]; 
        size_t symbol_size;
        handle_error(clingo_symbol_to_string(symbol, symbol_str_c, symbol_size));
        std::string symbol_str = std::string(symbol_str_c);
        

        handle_error(clingo_symbolic_atoms_literal(symbolic_atoms, symbolic_atoms_it, &plit));
        handle_error(clingo_propagate_init_solver_literal(init, plit, &slit));

        if (plit > max_plit) { max_plit = plit; }


        debug("symbol: ", symbol_str, " plit: ", plit, " slit: ", slit);
        // atoms_list_for_mapping.emplace_back(symbol_str, plit, slit);

        handle_error(clingo_symbolic_atoms_next(symbolic_atoms, symbolic_atoms_it, &symbolic_atoms_it));
    }


    return true ;
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
    AmoSumPropagator dummy_propagator ;
    for(const auto& [prop_type, param]: prop_type_params){
        // std::string param_str = unordered_map_to_string(param);
        // printf("%s with param: %s\n", prop_type.c_str(), param_str.c_str());
        register_propagator(ctl, prop, dummy_propagator);
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

void register_propagator(clingo_control_t *ctl, clingo_propagator_t prop, AmoSumPropagator& propagator){
  handle_error(clingo_control_register_propagator(ctl, &prop, &propagator, false));
}
