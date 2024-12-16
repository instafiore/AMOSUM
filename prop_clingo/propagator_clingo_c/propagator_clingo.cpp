#include <clingo.h>
#include <stdlib.h>
#include <stdio.h>
#include <string>
#include <assert.h>
#include "../../utility.h"
#include "../../amosum.h"
#include <sstream>
#include <iostream>

bool print_model(clingo_model_t const *model) {
  bool ret = true;
  clingo_symbol_t *atoms = NULL;
  size_t atoms_n;
  clingo_symbol_t const *it, *ie;
  char *str = NULL;
  size_t str_n = 0;
 
  // determine the number of (shown) symbols in the model
  if (!clingo_model_symbols_size(model, clingo_show_type_shown, &atoms_n)) { goto error; }
 
  // allocate required memory to hold all the symbols
  if (!(atoms = (clingo_symbol_t*)malloc(sizeof(*atoms) * atoms_n))) {
    clingo_set_error(clingo_error_bad_alloc, "could not allocate memory for atoms");
    goto error;
  }
 
  // retrieve the symbols in the model
  if (!clingo_model_symbols(model, clingo_show_type_shown, atoms, atoms_n)) { goto error; }
 
  printf("Model:");
 
  for (it = atoms, ie = atoms + atoms_n; it != ie; ++it) {
    size_t n;
    char *str_new;
 
    // determine size of the string representation of the next symbol in the model
    if (!clingo_symbol_to_string_size(*it, &n)) { goto error; }
 
    if (str_n < n) {
      // allocate required memory to hold the symbol's string
      if (!(str_new = (char*)realloc(str, sizeof(*str) * n))) {
        clingo_set_error(clingo_error_bad_alloc, "could not allocate memory for symbol's string");
        goto error;
      }
 
      str = str_new;
      str_n = n;
    }
 
    // retrieve the symbol's string
    if (!clingo_symbol_to_string(*it, str, n)) { goto error; }
 
    printf(" %s", str);
  }
 
  printf("\n");
  goto out;
 
error:
  ret = false;
 
out:
  if (atoms) { free(atoms); }
  if (str)   { free(str); }
 
  return ret;
}

bool solve(clingo_control_t *ctl, clingo_solve_result_bitset_t *result) {
  bool ret = true;
  clingo_solve_handle_t *handle;
  clingo_model_t const *model;
 
  // get a solve handle
  if (!clingo_control_solve(ctl, clingo_solve_mode_yield, NULL, 0, NULL, NULL, &handle)) { goto error; }
  // loop over all models
  while (true) {
    if (!clingo_solve_handle_resume(handle)) { goto error; }
    if (!clingo_solve_handle_model(handle, &model)) { goto error; }
    // print the model
    if (model) { print_model(model); }
    // stop if there are no more models
    else       { break; }
  }
  // close the solve handle
  if (!clingo_solve_handle_get(handle, result)) { goto error; }
 
  goto out;
 
error:
  ret = false;
 
out:
  // free the solve handle
  return clingo_solve_handle_close(handle) && ret;
}


bool init(clingo_propagate_init_t *init, AmoSumPropagator *propagator){
    return true ;
}

bool propagate(clingo_propagate_control_t *control, const clingo_literal_t *changes, size_t size, AmoSumPropagator *propagator){
    return true;
}

void undo(clingo_propagate_control_t *control, const clingo_literal_t *changes, size_t size, AmoSumPropagator *propagator){

}



int main(int argc, char const *argv[])
{
    printf("propagator_clingo.cpp started\n");

    std::unordered_map<std::string, std::string> params =  init_param(argc, argv);
    // print_unordered_map(params);

    std::string encoding_path = params.find("enc")->second ;
    std::string instance_path = params.find("i")->second ;

    printf("encoding: %s\n", encoding_path.c_str());
    printf("instance: %s\n", instance_path.c_str());

    
    std::string encoding = cat(encoding_path);
    std::string instance = cat(instance_path);

    ParameterMap::iterator it_amosum = params.find("amosum_propagator")  ;

    std::vector<std::string> sys_parameters;
    if (it_amosum != params.end()){
        std::string sys_parameters_str = it_amosum->second ;
        sys_parameters = split(sys_parameters_str, ' ');
    }


    std::vector<std::pair<std::string, ParameterMap>> prop_type_params = process_sys_parameters(sys_parameters) ;
    for(const auto& [prop_type, param]: prop_type_params){
        std::string param_str = unordered_map_to_string(param);
        printf("%s with param: %s\n", prop_type.c_str(), param_str.c_str());
    }


    char const *error_message;
    int ret = 0;
    clingo_solve_result_bitset_t solve_ret;
    clingo_control_t *ctl = NULL;
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
    // user data for the propagator
    AmoSumPropagator propagator;

    // register the propagator
    // if (!clingo_control_register_propagator(ctl, &prop, &propagator, false)) { goto error; }
    

    return 0 ;
    clingo_control_load(ctl, encoding_path.c_str());
    clingo_control_load(ctl, instance_path.c_str());
    

    // ground the pigeon part
    if (!clingo_control_ground(ctl, parts, 1, NULL, NULL)) { goto error; }
    
    // solve using a model callback
    if (!solve(ctl, &solve_ret)) { goto error; }
    
    goto out;
    
    error:
    if (!(error_message = clingo_error_message())) { error_message = "error"; }
    
    printf("%s\n", error_message);
    ret = clingo_error_code();
    
    out:

    if (ctl) { clingo_control_free(ctl); }
    
    return ret;

    
}
