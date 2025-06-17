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
#include <chrono>

using ParameterMap = std::unordered_map<std::string, std::string>;
int Group::autoincrement = 0;

// Function to get the name
std::string get_name(const std::unordered_map<clingo_symbol_t, clingo_literal_t>* atomNames, clingo_literal_t lit) {
    std::string prefix = "";

    if (lit == SETTINGS::NONE) { // Assuming 0 represents None in this context
        return SETTINGS::NONE_STR;
    }

    if (lit < 0) {
        prefix = "not ";
    }

    for (const auto& [name, atom] : *atomNames) {
        if (atom == std::abs(lit)) {
            return prefix + from_symbol_to_string(name);
        }
    }

    debug(lit, " is not present in atomNames ", atomNames_to_string(atomNames));
    assert(false);
    return SETTINGS::NONE_STR; 
}

clingo_symbol_t from_string_to_symbol(std::string str, const std::unordered_map<clingo_symbol_t, clingo_literal_t> *atomNames){return from_string_to_symbol_or_lit(str, atomNames, true);}
clingo_literal_t from_string_to_lit(std::string str, const std::unordered_map<clingo_symbol_t, clingo_literal_t> *atomNames){return from_string_to_symbol_or_lit(str, atomNames, false);}
int64_t from_string_to_symbol_or_lit(std::string str, const std::unordered_map<clingo_symbol_t, clingo_literal_t> *atomNames, bool sym){
    
    // auto start = std::chrono::high_resolution_clock::now();
    for (const auto& [name, atom] : (*atomNames)) {
        if (str == from_symbol_to_string(name)) {
            // auto end = std::chrono::high_resolution_clock::now();
            // std::chrono::duration<double> elapsed = end - start;
            // debugf("duration from_symbol_to_string: ",elapsed.count());
            return sym ? name : atom; 
        }
    }
    
    throw std::runtime_error(str+" is not present in atomNames");
    return 0; 
}

std::map<std::string, clingo_literal_t> create_atomNames_string(const std::unordered_map<clingo_symbol_t, clingo_literal_t> *atomNames){
    std::map<std::string, clingo_literal_t> atomNamesString ;
    for (const auto& [name, atom] : (*atomNames)) {
        atomNamesString[from_symbol_to_string(name)] = atom ;
    }
    return std::move(atomNamesString); 
}

std::vector<clingo_literal_t> create_assumptions_lits(
    const std::vector<std::string>& assumptions_vec,
    const std::unordered_map<clingo_symbol_t, clingo_literal_t>* atomNames) {

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




std::string atomNames_to_string(const std::unordered_map<clingo_symbol_t, clingo_literal_t>* atomNames){
    std::ostringstream oss;
    oss<< "{" ;
    for (const auto& [key, value] : (*atomNames)) {
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


void print_unordered_map(const std::unordered_map<std::string, std::string>& map){
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
        if (model) { print_model(model);}
        // stop if there are no more models
        else       { 
        break;
        }
    }

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
        choice_cons = "EO";
        propagation_phase = propagation_phase_le_eo ;
    } else if (prop_type == "ge_eo") {
        ge = true;
        choice_cons = "EO";
        propagation_phase = propagation_phase_ge_eo ;
    } else {
        assert(false && "Unexpected prop_type!");
    }

    return std::make_tuple(ge, propagation_phase, choice_cons);
}

void create_reason_falses(AmoSumPropagator* propagator, bool ge, std::unordered_map<clingo_literal_t, int> &sum_removed_weights, clingo_literal_t flipped = SETTINGS::NONE) {
    // TODO: modify propagator->reason
    if (ge) {
        create_reason_falses_ge(propagator, sum_removed_weights, flipped);
    } else {
        create_reason_falses_le(propagator, flipped);
    }
}



#ifdef CHECK_MPS
void weights_names_log(const std::string& ID, const std::unordered_map<std::string, int>& weights_names) {
    
    nlohmann::json json_weights = weights_names; // Convert weights_names to JSON
    std::ostringstream oss;
    oss << "id: " << ID << " total_weight_names: " << json_weights.dump();
    debugf(oss.str());
    
}
#endif

void create_reason_falses_ge(AmoSumPropagator* propagator, std::unordered_map<clingo_literal_t, int> &sum_removed_weights, clingo_literal_t flipped = SETTINGS::NONE) {

    if(propagator->dl == 0) return ;

    std::unordered_map<int, bool> breaks;
    
    for (auto* g : propagator->groups) {
        if (propagator->true_group->get(g) == SETTINGS::NONE) {
            clingo_literal_t ml_g = m_w(g, propagator->ge);

            int mw_g = propagator->weight->get(ml_g);

            for (int i = static_cast<int>(g->ord_l.size()) - 1; i >= 0; --i) {
                clingo_literal_t l = g->ord_l[i];
                
                if (propagator->weight->get(l) < mw_g) break;
                if (!propagator->I->get(l) && !equals(l, flipped)) {
                    for(auto derived : propagator->S){
                        // ADDED
                        if(breaks.find(derived) != breaks.end() && breaks[derived]) continue;
                        
                        Group* G = propagator->group->get(derived);
                        bool derived_true = true;
                        if (!G) {
                            G = propagator->group->get(not_(derived));
                            derived_true = false;
                        }
                        assert(G != nullptr);
                        if(g == G) continue; 

                        auto mps_h = propagator->mps_violated ? propagator->_mps : std::get<0>(propagator->mps(G, derived, !derived_true));
                        int s = propagator->lb - mps_h - 1;
                        int weight = propagator->weight->get(l);
                        int inc = weight - mw_g;
                        // 

                        auto R = get_perfect_hash_with_pointer(propagator->reason.get(), derived);
        
                        get_map(sum_removed_weights, derived, 0, true);
                        if(sum_removed_weights[derived] + inc <= s){
                            sum_removed_weights[derived] += inc ;
                            breaks[derived] = true ;
                        }else{
                            R->push_back(l);
                        }
                        
                    }
                }
            }
        } 
        else if(!equals(propagator->true_group->get(g), flipped)) {
            int tr = propagator->true_group->get(g) ;
    
            for(auto derived : propagator->S){
                auto R = get_perfect_hash_with_pointer(propagator->reason.get(), derived);
                // ADDED
                Group* G = propagator->group->get(derived);
                bool derived_true = true;
                if (!G) {
                    G = propagator->group->get(not_(derived));
                    derived_true = false;
                }
                //
                if(g == G) continue; 

                

                // ADDED
                assert(G != nullptr);

                auto mps_h = propagator->mps_violated ? propagator->_mps : std::get<0>(propagator->mps(G, derived, !derived_true));

                int s = propagator->lb - mps_h - 1;
                int w = propagator->weight->get(tr); 
                int w_mw_g = g->ord_l.size() > 0 ? propagator->weight->get(g->ord_l.back()) : s + 1 + w; 
                int inc = w_mw_g - w;
                assert(inc >= 0);

                get_map(sum_removed_weights, derived, 0, true);
                if(sum_removed_weights[derived] + inc <= s){
                    sum_removed_weights[derived] += inc ;
                }else{
                    R->push_back(not_(tr));
                }
            }    
        }
    }
    

    // if(propagator->dl == 0) return ;
    
    // for (auto* g : propagator->groups) {
    //     if (propagator->true_group->get(g) == SETTINGS::NONE) {
    //         clingo_literal_t ml_g = m_w(g, propagator->ge);
    //         int mw_g = propagator->weight->get(ml_g);

    //         for (int i = static_cast<int>(g->ord_l.size()) - 1; i >= 0; --i) {
    //             clingo_literal_t l = g->ord_l[i];
    //             if (propagator->weight->get(l) < mw_g) break;
    //             if (!propagator->I->get(l) && !equals(l, flipped)) {
    //                 for(auto lit : propagator->S){
    //                     auto R = get_perfect_hash_with_pointer(propagator->reason.get(), lit);
    //                     Group* G = propagator->group->get(lit);
    //                     if (G == nullptr) G = propagator->group->get(not_(lit));
    //                     if(g == G) continue; 
    //                     R->push_back(l);
    //                 }
    //             }
    //         }
    //     } 
    //     else if(!equals(propagator->true_group->get(g), flipped)) {
    //         for(auto lit : propagator->S){
    //             auto R = get_perfect_hash_with_pointer(propagator->reason.get(), lit);
    //             Group* G = propagator->group->get(lit);
    //             if (G == nullptr) G = propagator->group->get(not_(lit));
    //             if(g == G) continue; 
    //             R->push_back(not_(propagator->true_group->get(g)));
    //         }
    //     }
    // }

}

void create_reason_true_ge(AmoSumPropagator* propagator, clingo_literal_t sml_g, clingo_literal_t derived, Group* g, std::unordered_map<clingo_literal_t, int> &sum_removed_weights){
    if(propagator->dl == 0) return ;
    
    int i = sml_g != SETTINGS::NONE ? g->ord_i[sml_g] : 0;
    int j = g->ord_l.size() -1;
    // int j = g->ord_l.size(); 

    auto R = get_perfect_hash_with_pointer(propagator->reason.get(), derived);

    assert(i <= j);
    assert(derived != SETTINGS::NONE);

    auto mps_h = propagator->mps_violated ? propagator->_mps : std::get<0>(propagator->mps(g, derived, false));
    int s = propagator->lb - mps_h - 1 ;
    // for (int k = i; k < j; ++k) {
    for (int k = j; k >= i; --k) {
        clingo_literal_t lit = g->ord_l[k];
        int weight = propagator->weight->get(lit);
        int w_sml = propagator->weight->get(sml_g);
        int inc = weight - w_sml ;
        if (!propagator->I->get(lit) && !equals(derived, lit)) {
            get_map(sum_removed_weights, derived, 0, true);
            if(sum_removed_weights[derived] + inc <= s){
                sum_removed_weights[derived] += inc;
                break;
            }else
                R->push_back(lit);
            // R->push_back(lit);
        }
    }

    //     if(propagator->dl == 0) return ;
    
    // int i = sml_g != SETTINGS::NONE ? g->ord_i[sml_g] : 0;
    // int j = g->ord_l.size();

    // auto R = get_perfect_hash_with_pointer(propagator->reason.get(), derived);

    // assert(i <= j);
    // assert(derived != SETTINGS::NONE);
    
    // for (int k = i; k < j; ++k) {
    //     clingo_literal_t lit = g->ord_l[k];
    //     if (!propagator->I->get(lit) && !equals(derived, lit)) {
    //         R->push_back(lit);
    //     }
    // }
}


void create_reason_falses_le(AmoSumPropagator* propagator, clingo_literal_t flipped = SETTINGS::NONE) {
    if(propagator->dl == 0) return ;

    for (auto* g : propagator->groups) {
        if (propagator->true_group->get(g) == SETTINGS::NONE) {
            clingo_literal_t ml_g = m_w(g, propagator->ge); 
            int mw_g = propagator->weight->get(ml_g);
            
            for (clingo_literal_t l : g->ord_l) {
                if (propagator->weight->get(l) > mw_g) break;
                if (!propagator->I->get(l) && !equals(l, flipped)) {
                    for(auto lit : propagator->S){
                        auto R = get_perfect_hash_with_pointer(propagator->reason.get(), lit);
                        Group* G = propagator->group->get(lit);
                        if (G == nullptr) G = propagator->group->get(not_(lit));
                        if(g == G) continue; 
                        R->push_back(l);
                    }
                }
            }
        } else if(!equals(propagator->true_group->get(g), flipped)) {
            for(auto lit : propagator->S){
                auto R = get_perfect_hash_with_pointer(propagator->reason.get(), lit);
                Group* G = propagator->group->get(lit);
                if (G == nullptr) G = propagator->group->get(not_(lit));
                if(g == G) continue; 
                R->push_back(not_(propagator->true_group->get(g)));
            }
        }
    }
}

std::string vector_lit_to_string(const std::unordered_map<clingo_symbol_t, clingo_literal_t>* atomNames, const std::vector<clingo_literal_t>& vec, std::string name = ""){
    std::ostringstream oss;
    int n = vec.size() ;
    
    oss<<name<<"[";
    for (int i = 0; i < n-1; i++) oss << "'" <<get_name(atomNames, vec[i])<< "'" << ",";
    if (n > 0) oss<<"'"<<get_name(atomNames, vec[n-1])<<"'";

    oss<<"]";
    return oss.str();
}



void create_reason_true_le(AmoSumPropagator* propagator, clingo_literal_t sml_g, clingo_literal_t derived, Group* g){
    if(propagator->dl == 0) return ;
    
    int i = sml_g != SETTINGS::NONE ? g->ord_i[sml_g] : g->ord_l.size() - 1;
    int j = 0;

    auto R = get_perfect_hash_with_pointer(propagator->reason.get(), derived);

    assert(i >= j);
    assert(derived != SETTINGS::NONE);
    
    for (int k = i; k >= j; --k) {
        clingo_literal_t lit = g->ord_l[k];
        if (!propagator->I->get(lit) && !equals(derived, lit)) {
            R->push_back(lit);
        }
    }
}


void raise_exception(std::string message){
    throw std::runtime_error(message);
}

void raise_wasp_not_implemented_exception(){
    raise_exception("wasp not implemented yet");
}




void print_derivation(const std::unordered_map<clingo_symbol_t, clingo_literal_t>* atomNames, const std::vector<clingo_literal_t>& S, bool force_print = false){
    bool debug_b = false ;
    #ifdef DEBUG
        debug_b = true ;
    #endif
    if (not force_print and not debug_b) return ;

    debugf(vector_lit_to_string(atomNames, S, "Derived"));
}



void print_reason(const std::unordered_map<clingo_symbol_t, clingo_literal_t>* atomNames, const std::vector<clingo_literal_t>& R, clingo_literal_t lit, bool force_print = false){
    bool debug_b = false ;
    #ifdef DEBUG
        debug_b = true ;
    #endif
    if (not force_print and not debug_b) return ;

    std::string reason_name = "Reason("+std::to_string(lit)+") ";
    debug(vector_lit_to_string(atomNames, R, reason_name));
}


void print_reduction_reason(const AmoSumPropagator& propagator,
                            const std::vector<clingo_literal_t>& reason_c,
                            const std::vector<clingo_literal_t>& reason,
                            clingo_literal_t lit, float p,
                            bool force_print = false) {
    bool debug_b = false;
    #ifdef DEBUG
        debug_b = true;
    #endif;
    
    if (!force_print && !debug_b) return;
    
    // Generate redundant literals string for the first message
    std::string redundant_lits_str = "from " + std::to_string(reason_c.size()) + " to " + std::to_string(reason.size()) +
        " with p: " + std::to_string(p) + "%";

    debugf(redundant_lits_str);
}



std::chrono::time_point<std::chrono::high_resolution_clock> start_timer(){
    return std::chrono::high_resolution_clock::now(); 
}

void display_end_timer(const std::chrono::time_point<std::chrono::high_resolution_clock>& start, std::string name){
    auto end = std::chrono::high_resolution_clock::now(); 
    std::chrono::duration<double> elapsed = (end - start);
    debugf(name," time: ", elapsed.count(), "s");
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
        if(prop->map_slit_plit->find(decision_slit) == prop->map_slit_plit->end()){
            decision_slit = -1 ;
        }else {
            plit = (*prop->map_slit_plit)[decision_slit][0];
        }
    }else if (wasp_b)
    {
        raise_wasp_not_implemented_exception();
    }
    
    std::string decision_literal_name ; 
    if(decision_slit != -1)
        decision_slit != 1 ? decision_literal_name = get_name(prop->atomNames, plit) : decision_literal_name = "from facts" ;
    else
        decision_literal_name = "non lo so";
    debugf("[", decision_literal_name,", ",dl,"] propagate ", changes_str, " mps: ", prop->propagators[td]->_mps, " bound: ",prop->propagators[td]->bound," td: ", td);
}

void print_undo(PropagatorClingo* prop, const clingo_literal_t *changes, size_t size, clingo_propagate_control_t *control, int dl, int td, bool force_print = false, bool wasp_b = false){
    bool debug_b = false ;
    #ifdef DEBUG
        debug_b = true ;
    #endif
    if (not force_print and not debug_b) return ;


    std::string changes_str ;
    
    if (wasp_b)  raise_wasp_not_implemented_exception() ;
    else  changes_str = prop->compute_changes_str(changes, size, td) ;


    debugf("dl: ",dl," undo ", changes_str," thread_id: ", td);
}

clingo_literal_t max_w(const Group* g) {
    if (g->max_und == SETTINGS::NONE) return SETTINGS::NONE; // No max undefined value 

    try {
        return g->ord_l[g->max_und]; // Get the literal using max_und
    } catch (const std::out_of_range& e) {
        debug("Error accessing g.ord_l with max_und. Debug info:");
        debug(vector_to_string(g->ord_l,"g.ord_l: "));
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


int increment_f(bool derived_true, clingo_literal_t l, const std::unordered_set<clingo_literal_t>& current_subset_maximal, const WeightFunction*& weight, const GroupFunction*& group, int head_reason, const std::unique_ptr<InterpretationFunction>& I, int max_b) {
    Group* g = group->get(l);
    bool tr = false;

    if (!g) {
        l = not_(l);
        g = group->get(l);
        tr = true;
    }

    Group* head_group = group->get(head_reason);
    if(head_group == nullptr)
        head_group = group->get(not_(head_reason));
    assert(head_group != nullptr);
    int w = weight->get(l);
    if (tr) {
        assert(!g->ord_l.empty());
        int w_mw_g = weight->get(g->ord_l.back());
        assert(g != head_group);
        assert(w_mw_g >= w);
        return w_mw_g - w;
    } else {
        int i = g->ord_l.size() - 1;
        int mw_g;
       
        if (g == head_group) {
            
            if(!derived_true) return 0 ; // by default a literal of the same group of derived not l, where l in aggregate, is redundant
            auto [sml_g, ml_g] = g->update(I, max_b, false, false, SETTINGS::NONE);
            mw_g = weight->get(sml_g);
        } else {
            mw_g = weight->get(max_w(g));
        }

        int current_l = g->ord_l[i];
        int increment = w - mw_g;
        assert(w >= mw_g);
        while (mw_g < weight->get(current_l)) {
            if (current_subset_maximal.find(current_l) != current_subset_maximal.end()) {
                increment = std::max(0, w - weight->get(current_l));
                break;
            }
            if (--i <= 0) break;
            current_l = g->ord_l[i];
        }

        return increment;
    }
}

void remove_elements(std::vector<clingo_literal_t>& original, const std::unordered_set<clingo_literal_t>& to_remove_set) {
 

    auto end_old = original.end() ;
    auto end_new = std::remove_if(original.begin(), original.end(),
                       [&to_remove_set](int element) {
                           return to_remove_set.find(element) != to_remove_set.end();
                       });
    while(end_new != end_old){
        original.pop_back();
        ++end_new ;
    }
    
    // auto original_c = original ;
    // original.clear();
    // for (size_t i = 0; i < original_c.size(); ++i)
    // {
    //     clingo_literal_t lit = original_c[i];
    //     if(to_remove_set.find(lit) == to_remove_set.end()){
    //         original.push_back(lit);
    //     }
    // }
    
}

// Maximal subset with groups
void maximal_subset_sum_less_than_s_with_groups(bool derived_true, const std::vector<clingo_literal_t>& literals, int s,const WeightFunction* weight, const GroupFunction* group, int head_reason, const std::unique_ptr<InterpretationFunction>& I, int max, std::unordered_set<clingo_literal_t>& current_subset_maximal) {
    int current_sum = 0;
    current_subset_maximal.clear();
    for (int l : literals) {
        int inc = increment_f(derived_true, l, current_subset_maximal, weight, group, head_reason, I, max);
        if (current_sum + inc <= s) {
            current_sum += inc;
            current_subset_maximal.emplace(l);
        }
    }
}

// Compute increment literals
std::unordered_map<int, int> compute_increment_literals(const std::vector<int>& literals,
                                                        const GroupFunction* group,
                                                        const WeightFunction* weight) {
    std::unordered_map<int, int> increment;

    for (int l : literals) {
        Group* g = group->get(l);
        bool tg = false;

        if (!g) {
            g = group->get(not_(l));
            tg = true;
        }

        int mw_g = max_w(g);
        int w_mw_g = weight->get(mw_g);
        int w = weight->get(l);

        int i = tg ? (weight->get(g->ord_l.back()) - w) : (w - w_mw_g);
        increment[l] = i;
    }
    return increment;
}

// Get all literals below a literal
std::vector<clingo_literal_t> get_all_lit_below_you(int lit, const GroupFunction* group,
                                    const InterpretationFunction* I, const std::vector<int>& reason) {
    std::vector<clingo_literal_t> res{lit};
    Group* g = group->get(lit);

    if (!g) {
        g = group->get(not_(lit));
        return res;
    }

    int start = g->ord_i[lit];

    for (int i = start - 1; i >= 0; --i) {
        int l = g->ord_l[i];
        if (I->get(l) == 0) break;
        if (std::find(reason.begin(), reason.end(), l) != reason.end()) {
            res.push_back(l);
        }
    }
    return res;
}

// Maximum subset sum with groups
std::unordered_set<clingo_literal_t> maximum_subset_sum_less_than_s_with_groups(const std::vector<clingo_literal_t>& literals, int s,
                                                            const GroupFunction* group,
                                                            const WeightFunction* weight,
                                                            const InterpretationFunction* I) {
    int n = literals.size();
    std::vector<std::vector<std::vector<clingo_literal_t>>> subset(s + 1, std::vector<std::vector<clingo_literal_t>>(n + 1));

    for (int i = 0; i <= n; ++i) {
        subset[0][i] = {};
        if (i == 0) continue;
        int l = literals[i - 1];
        subset[0][i] = (weight->get(l) == 0) ? subset[0][i - 1] : subset[0][i - 1];
    }

    for (int i = 1; i <= s; ++i) {
        for (int j = 1; j <= n; ++j) {
            int lit = literals[j - 1];
            int w = weight->get(lit);
            subset[i][j] = subset[i][j - 1];

            if (i >= w && !subset[i - w][j - 1].empty()) {
                subset[i][j] = std::max(subset[i][j],
                                        subset[i - w][j - 1],
                                        [](const std::vector<clingo_literal_t>& a, const std::vector<clingo_literal_t>& b) {
                                            return a.size() < b.size();
                                        });
            }
        }
    }
    return std::unordered_set<clingo_literal_t>(subset[s][n].begin(), subset[s][n].end());
}