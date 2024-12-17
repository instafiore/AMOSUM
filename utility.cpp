#include <unordered_map>
#include <iostream>
#include <vector>
#include <string>
#include <regex>
#include <stdexcept>
#include "settings.h"
#include "utility.h"
#include <sstream>
#include <clingo.h>
#include <fstream>

using ParameterMap = std::unordered_map<std::string, std::string>;


// Explicit template instantiations for compilation
template std::string vector_to_string<std::string>(const std::vector<std::string>& v);

template <typename T>
std::string vector_to_string(const std::vector<T>& vec){
    std::ostringstream oss;
    oss<<"[";
    for (size_t i = 0; i < vec.size()-1; i++)
    {
        oss<<"'"<<vec[i]<<"'"<<"," ;
    }
    if (vec.size() > 0) oss<<"'"<<vec[vec.size()-1]<<"'";

    oss<<"]";
    return oss.str();
}


ParameterMap init_param(int argc, char const *argv[]) {

    ParameterMap args;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        size_t pos = arg.find('=');
        if (pos != std::string::npos && arg[0] == '-') {
            std::string key = arg.substr(1, pos - 1); 
            std::string value = arg.substr(pos + 1); 
            args[key] = value;
        } else if (arg[0] == '-'){
            std::string key = arg.substr(1);
            args[key] = "true";
        } else {
            std::cerr << "Invalid argument format: " << arg << "\n";
        }
    }

    return args;
}

std::string unordered_map_to_string(std::unordered_map<std::string, std::string> map){

    std::ostringstream oss;
    oss<< "{" ;
    for (const auto& [key, value] : map) {
        oss <<"'"<< key << "':'" << value<<"', " ;
    }
    oss << "}" ;
    return oss.str() ;
}

void print_unordered_map(std::unordered_map<std::string, std::string> map){
    std::cout << unordered_map_to_string(map) << "\n" ; 
}



std::vector<std::pair<std::string, ParameterMap>> process_sys_parameters(const std::vector<std::string>& sys_parameters) {

    if (sys_parameters.empty()) {
        return std::vector<std::pair<std::string, ParameterMap>>();
    }

    std::vector<std::pair<std::string, ParameterMap>> params;
    std::regex regex("^-(.+)");
    size_t i = 0;
    std::string prop_type = sys_parameters[i++];
    ParameterMap param;

    // printf("[process_sys_parameters]i: %zu sys_parameters: %s\n", i, vector_to_string(sys_parameters).c_str());

    while (i < sys_parameters.size()) {
        
        // Current key
        std::string key = sys_parameters[i];
        std::smatch match;

        if (!std::regex_match(key, match, regex)) {
            throw std::runtime_error("Every key has to start with a dash! Ex: -id id");
        }
        key = match[1];

        // Check the next value
        if (i + 1 >= sys_parameters.size()) {
            param[key] = "true"; // Default value for standalone keys
            i++;
        } else {
            std::string value = sys_parameters[i + 1];
            if (!std::regex_match(value, match, regex) && std::find(PROPAGATORS_NAMES.begin(), PROPAGATORS_NAMES.end(), value) == PROPAGATORS_NAMES.end()) {
                param[key] = value;
                i += 2;
            } else {
                i++ ;
                param[key] = "true";
            }
        }

        if (i >= sys_parameters.size() || 
            std::find(PROPAGATORS_NAMES.begin(), PROPAGATORS_NAMES.end(), sys_parameters[i]) != PROPAGATORS_NAMES.end()) {
            params.emplace_back(prop_type, param);
            if (i < sys_parameters.size()) {
                prop_type = sys_parameters[i];
                param.clear();
                i++;
            }
        }
    }

    return params;
}

std::vector<std::string> split(const std::string& str, char delimiter) {
    std::vector<std::string> tokens;
    std::istringstream stream(str);
    std::string token;

    while (std::getline(stream, token, delimiter)) {
        tokens.push_back(token);
    }

    return tokens;
}


// returns the offset'th numeric argument of the function symbol sym
bool get_arg(clingo_symbol_t sym, int offset, int *num) {
  clingo_symbol_t const *args;
  size_t args_size;
 
  // get the arguments of the function symbol
  if (!clingo_symbol_arguments(sym, &args, &args_size)) { return false; }
  // get the requested numeric argument
  if (!clingo_symbol_number(args[offset], num)) { return false; }
 
  return true;
}

std::string cat(const std::string &filename) {
    std::ifstream file(filename); 
    if (!file.is_open()) {
        std::cerr << "Error: Could not open file " << filename << std::endl;
        return "";
    }

    std::ostringstream oss ;
    std::string line;
    while (std::getline(file, line)) { 
        oss << line << '\n';    
    }

    file.close(); 
    return oss.str() ;
}


void handle_error(bool success) {
    char const *error_message;
    if (!success) {
        if (!(error_message = clingo_error_message())) { error_message = "generic error, not recognized from clingo"; }
        fprintf(stderr, "Error: %s\n", error_message);
        exit(EXIT_FAILURE);
    }
}

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
 
  printf("Answer set {");
 
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
 
    it+1 != ie ? printf("%s, ", str) : printf(" %s", str);

  }
 
  printf("}\n");
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
  handle_error(clingo_control_solve(ctl, clingo_solve_mode_yield, NULL, 0, NULL, NULL, &handle));
  // loop over all models
  while (true) {
    handle_error(clingo_solve_handle_resume(handle));
    handle_error(clingo_solve_handle_model(handle, &model));
    // print the model
    if (model) { print_model(model); }
    // stop if there are no more models
    else       { break; }
  }
  // close the solve handle
  handle_error(clingo_solve_handle_get(handle, result));
 
  return clingo_solve_handle_close(handle) && ret;
}