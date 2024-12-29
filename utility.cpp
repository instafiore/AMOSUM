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
#include <cassert>
#include "amosum.h"
#include "prop_wasp/propagator_wasp_c/ge_amo.h" 
#include "prop_wasp/propagator_wasp_c/ge_eo.h" 
#include "prop_wasp/propagator_wasp_c/le_eo.h" 
#include "prop_clingo/propagator_clingo_c/propagator_clingo.h"
#include "utility.tpp"

using ParameterMap = std::unordered_map<std::string, std::string>;
int Group::autoincrement = 0;

// Function to get the name
std::string get_name(const std::unordered_map<clingo_symbol_t, clingo_literal_t>& atomNames, clingo_literal_t lit) {
    std::string prefix = "";

    if (lit == SETTINGS::NONE) { // Assuming 0 represents None in this context
        return SETTINGS::NONE_STR;
    }

    if (lit < 0) {
        prefix = "not ";
    }

    for (const auto& [name, atom] : atomNames) {
        if (atom == std::abs(lit)) {
            return prefix + from_symbol_to_string(name);
        }
    }

    debug(lit, " is not present in atomNames");
    assert(false);
    return SETTINGS::NONE_STR; 
}

clingo_symbol_t from_string_to_symbol(std::string str, const std::unordered_map<clingo_symbol_t, clingo_literal_t> &atomNames){return from_string_to_symbol_or_lit(str, atomNames, true);}
clingo_literal_t from_string_to_lit(std::string str, const std::unordered_map<clingo_symbol_t, clingo_literal_t> &atomNames){return from_string_to_symbol_or_lit(str, atomNames, false);}
int64_t from_string_to_symbol_or_lit(std::string str, const std::unordered_map<clingo_symbol_t, clingo_literal_t> &atomNames, bool sym){
  
    for (const auto& [name, atom] : atomNames) {
        if (str == from_symbol_to_string(name)) {
            return sym ? name : atom; 
        }
    }

    throw std::runtime_error(str+" is not present in atomNames");
    return 0; 
}

std::vector<clingo_literal_t> create_assumptions_lits(
    const std::vector<std::string>& assumptions_vec,
    const std::unordered_map<clingo_symbol_t, clingo_literal_t>& atomNames) {

    std::vector<clingo_literal_t> res;

    // Regex for atom extraction
    const std::regex REGEX_LIT(std::to_string(SETTINGS::NOT)+"?(\\w+(\\(\\w+("+std::to_string(SETTINGS::SEPARATOR_ASSUMPTIONS)+"\\w+)*\\))?)");

    for (const auto& ass : assumptions_vec) {
        std::smatch match;
        if (std::regex_match(ass, match, REGEX_LIT)) {
            std::string atom = match[1].str(); // Extracted atom name
            try{
                clingo_literal_t lit = from_string_to_lit(ass, atomNames);
                if (ass.front() == SETTINGS::NOT) lit = not_(lit); // Negate if assumption starts with '~'
                res.push_back(lit);
            }catch (const std::runtime_error& e) {}
        }
    }
    return res;
}




std::string atomNames_to_string(std::unordered_map<clingo_symbol_t, clingo_literal_t> atomNames){
    std::ostringstream oss;
    oss<< "{" ;
    for (const auto& [key, value] : atomNames) {
        oss <<"'"<< from_symbol_to_string(key) << "':'" << value<<"', " ;
    }
    oss << "}" ;
    return oss.str() ;
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
            args[key] = SETTINGS::TRUE_STR;
        } else {
            std::cerr << "Invalid argument format: " << arg << "\n";
        }
    }

    return args;
}


void print_unordered_map(std::unordered_map<std::string, std::string> map){
    std::cout << unordered_map_to_string(map) << "\n" ; 
}

std::string from_symbol_to_string(clingo_symbol_t symbol){
    size_t symbol_size;
    handle_error(clingo_symbol_to_string_size(symbol, &symbol_size));
    char symbol_str_c[symbol_size]; 
    handle_error(clingo_symbol_to_string(symbol, symbol_str_c, symbol_size));
    std::string symbol_str = std::string(symbol_str_c);
    return symbol_str ;
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
            if (!std::regex_match(value, match, regex) && std::find(SETTINGS::PROPAGATORS_NAMES.begin(), SETTINGS::PROPAGATORS_NAMES.end(), value) == SETTINGS::PROPAGATORS_NAMES.end()) {
                param[key] = value;
                i += 2;
            } else {
                i++ ;
                param[key] = "true";
            }
        }

        if (i >= sys_parameters.size() || 
            std::find(SETTINGS::PROPAGATORS_NAMES.begin(), SETTINGS::PROPAGATORS_NAMES.end(), sys_parameters[i]) != SETTINGS::PROPAGATORS_NAMES.end()) {
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


std::string cat(const std::string &filename) {
    std::ifstream file(filename); 
    if (!file.is_open()) {
        std::cerr << "Error: Could not open file " << filename << std::endl;
        return SETTINGS::NONE_STR;
    }

    std::ostringstream oss ;
    std::string line;
    while (std::getline(file, line)) { 
        oss << line << '\n';    
    }

    file.close(); 
    return oss.str() ;
}

const std::string clingo_error_code_to_string(clingo_error_t code) {
    switch (code) {
        case clingo_error_bad_alloc:
            return "memory could not be allocated";
        case clingo_error_unknown:
            return "errors unrelated to clingo";
        case clingo_error_success:
            return "successful API calls";
        case clingo_error_runtime:
            return "errors only detectable at runtime like invalid input";
        case clingo_error_logic:
            return "wrong usage of the clingo API";
        default:
            return "unknown_error code: ";
    }
}

void handle_error(bool success) {
    char const *error_message;
    if (!success) {
        if (!(error_message = clingo_error_message())) { error_message = "generic error, not recognized from clingo"; }
        clingo_error_t code = clingo_error_code();
        std::string code_str = clingo_error_code_to_string(code);
        fprintf(stderr, "Clingo error: %s with code: %s\n", error_message, code_str.c_str());
        exit(code);
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
 
    it+1 != ie ? printf("%s, ", str, n) : printf(" %s", str);

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

std::tuple<bool, const std::vector<clingo_literal_t>* (*)(const Group*, AmoSumPropagator*), std::string>  get_propagator_variables(std::string prop_type){
    
    bool ge;
    std::string choice_cons;
    const std::vector<clingo_literal_t>* (*propagation_phase)(const Group*, AmoSumPropagator*);

    if (prop_type == "ge_amo") {
        ge = true;
        choice_cons = "AMO";
        propagation_phase = propagation_phase_ge_amo ;
    } else if (prop_type == "le_eo") {
        ge = false;
        choice_cons = "AMO";
        propagation_phase = propagation_phase_ge_eo ;
    } else if (prop_type == "ge_eo") {
        ge = true;
        choice_cons = "EO";
        propagation_phase = propagation_phase_le_eo ;
    } else {
        assert(false && "Unexpected prop_type!");
    }

    // Print for debugging purposes
    std::cout << "ge: " << ge << ", choice_cons: " << choice_cons << std::endl;
    
    return std::make_tuple(ge, propagation_phase, choice_cons);
}

void create_reason_falses(AmoSumPropagator* propagator, bool ge) {
    // TODO: modify propagator->reason
    if (ge) {
        create_reason_falses_ge(propagator);
    } else {
        create_reason_falses_le(propagator);
    }
}

void create_reason_falses_ge(AmoSumPropagator* propagator) {
    // TODO: modify propagator->reason
}

void create_reason_falses_le(AmoSumPropagator* propagator) {
    // TODO: modify propagator->reason
}

void raise_exception(std::string message){
    throw std::runtime_error(message);
}

void raise_wasp_not_implemented_exception(){
    raise_exception("wasp not yet implemented");
}


void print_propagate(PropagatorClingo* prop, const clingo_literal_t *changes, size_t size, clingo_propagate_control_t *control, int dl, bool force_print = false, bool wasp_b = false){
    bool debug_b = false ;
    #ifdef DEBUG
        debug_b = true ;
    #endif
    if (not force_print and not debug_b) return ;
    
    int td ;
    wasp_b ? td = 0 : td = clingo_propagate_control_thread_id(control) ; 
    std::string changes_str ;
    
    if (wasp_b)  raise_wasp_not_implemented_exception() ;
    else  changes_str = prop->compute_changes_str(changes, size, td) ;

    const clingo_assignment_t *assignment = clingo_propagate_control_assignment(control);
    clingo_literal_t decision_slit ;
    handle_error(clingo_assignment_decision(assignment, dl, &decision_slit));

    clingo_literal_t plit = 0 ;

    if (not wasp_b and decision_slit != 1){
        plit = prop->map_slit_plit[decision_slit][0];
    }else if (wasp_b)
    {
        raise_wasp_not_implemented_exception();
    }
    
    std::string decision_literal_name ; 
    decision_slit != 1 ? decision_literal_name = get_name(prop->atomNames, plit) : decision_literal_name = "from facts" ;

    debugf("[", decision_literal_name,", ",dl,"] propagate ", changes_str," td: ", td);
}


clingo_literal_t max_w(const Group* g) {
    if (g->max_und == SETTINGS::NONE) return SETTINGS::NONE; // No max undefined value 

    try {
        return g->ord_l[g->max_und]; // Get the literal using max_und
    } catch (const std::out_of_range& e) {
        debug("Error accessing g.ord_l with max_und. Debug info:");
        debug("g.ord_l: ", vector_to_string(g->ord_l));
        debug("max_und: " + std::to_string(g->max_und));
        throw; // Re-throw the exception
    }
}


// Function to return the min undefined literal
clingo_literal_t min_w(const Group* g) {
    if (g->min_und == SETTINGS::NONE) return SETTINGS::NONE; // No max undefined value 
    return g->ord_l[g->min_und]; // Get the literal using min_und
}

// Function to select between max_w and min_w
clingo_literal_t m_w(const Group* g, bool max) {
    return max ? max_w(g) : min_w(g) ; 
}

bool equals(const clingo_literal_t& l1, const clingo_literal_t& l2){
    if(l1 == SETTINGS::NONE || l2 == SETTINGS::NONE)
        return l1 == SETTINGS::NONE && l2 == SETTINGS::NONE ;
    return abs(l1) == abs(l2) ;
}

std::pair<clingo_literal_t, clingo_literal_t> Group::update_max(
    const std::unique_ptr<InterpretationFunction>& I, bool all = false, bool update = true, 
    const clingo_literal_t& assuming_und = SETTINGS::NONE) {

    clingo_literal_t prev_max = (max_und < ord_l.size())? ord_l[max_und]: SETTINGS::NONE;

    int start = all ? (N - 1) : (max_und != SETTINGS::NONE ? (max_und - 1) : -1);
    if (start < 0) {
        if (update) max_und = SETTINGS::NONE;
        return {SETTINGS::NONE, prev_max};
    }

    for (int i = start; i >= 0; --i) {
        clingo_literal_t l = ord_l[i];
        if (I->get(l) == SETTINGS::NONE || equals(l, assuming_und)) {
            if (update) max_und = i;
            return {l, prev_max};
        }
    }

    if (update) max_und = SETTINGS::NONE;
    return {SETTINGS::NONE, prev_max};
}

std::pair<clingo_literal_t, clingo_literal_t> Group::update_min(
        const std::unique_ptr<InterpretationFunction>& I, bool all = false, bool update = true, 
        const clingo_literal_t& assuming_und = SETTINGS::NONE) {

        clingo_literal_t prev_min = (min_und < ord_l.size()) ? ord_l[min_und] : SETTINGS::NONE;

        int start = all ? 0 : (min_und != SETTINGS::NONE ? (min_und + 1) : N);
        if (start >= N) {
            if (update) min_und = SETTINGS::NONE;
            return {SETTINGS::NONE, prev_min};
        }

        for (int i = start; i < N; ++i) {
            clingo_literal_t l = ord_l[i];
            if (I->get(l) == SETTINGS::NONE || equals(l, assuming_und)) {
                if (update) min_und = i;
                return {l, prev_min};
            }
        }

        if (update) min_und = SETTINGS::NONE;
        return {SETTINGS::NONE, prev_min};
    }

    std::pair<clingo_literal_t, clingo_literal_t> Group::update(
        const std::unique_ptr<InterpretationFunction>& I, bool max, bool all = false, bool update = true, 
        const clingo_literal_t& assuming_und = SETTINGS::NONE) {

        if (max) {
            return update_max(I, all, update, assuming_und);
        } else {
            return update_min(I, all, update, assuming_und);
        }
    }


std::vector<std::string> convert_assparam_to_assarray(const std::string& assumptions) {
        if(assumptions == SETTINGS::NONE_STR) return std::vector<std::string>();
        std::string stripped_string = assumptions;
        stripped_string.erase(0, stripped_string.find_first_not_of("[]"));
        stripped_string.erase(stripped_string.find_last_not_of("[]") + 1);

        std::vector<std::string> array;
        std::istringstream ss(stripped_string);
        std::string item;

        while (std::getline(ss, item, SETTINGS::SEPARATOR_ASSUMPTIONS)) {
            array.push_back(item);
        }

        return array;
}

void simplifyLiterals(
    std::vector<clingo_literal_t>& lits,
    AggregateFunction* aggregate,
    GroupFunction* group,
    bool max,
    const std::unique_ptr<InterpretationFunction>& I
) {
    Group* G = nullptr;

    for (auto l : lits) {
        if (aggregate->get(l)) {
            G = group->get(l);
            G->add_false_lit(not_(l));
        } else if (aggregate->get(not_(l))) {
            G = group->get(not_(l));
            G->add_false_lit(l);
            l = not_(l);
        } else {
            continue;
        }
        // continue;

        int n = G->ord_l.size();
        int li = G->ord_i[l];
        G->ord_i[l] = -1;

        // Updating positions of next literals (shifting)
        for (int lit_i = li + 1; lit_i < n; ++lit_i) {
            clingo_literal_t lit = G->ord_l[lit_i];
            G->ord_i[lit] -= 1;
        }

        // Removing the literal
        auto it = std::find(G->ord_l.begin(), G->ord_l.end(), l);
        if (it != G->ord_l.end()) {
            G->ord_l.erase(it);
        }
        G->N = G->ord_l.size();

        // Updating max_und/min_und
        G->update(I, max, true);

        // Removing from aggregate
        aggregate->set(l,false);
        aggregate->set(not_(l),false);

        assert(G->count_undef >= 0);
    }
}
