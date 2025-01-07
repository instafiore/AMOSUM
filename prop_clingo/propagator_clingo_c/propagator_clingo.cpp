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

bool PropagatorClingo::init(clingo_propagate_init_t *init){

    debug("starting propagator with params: ", unordered_map_to_string(this->param))
    
    size_t nt = clingo_propagate_init_number_of_threads(init);
    debug("[init] number of threads ", nt)

    clingo_literal_t max_plit = 0;
    clingo_symbolic_atoms_t const *symbolic_atoms;
    clingo_symbolic_atom_iterator_t symbolic_atoms_it, symbolic_atoms_ie;
    
    handle_error(clingo_propagate_init_symbolic_atoms(init, &symbolic_atoms));
    handle_error(clingo_symbolic_atoms_end(symbolic_atoms, &symbolic_atoms_ie));

    handle_error(clingo_symbolic_atoms_begin(symbolic_atoms, NULL, &symbolic_atoms_it));


    std::vector<std::tuple<clingo_symbol_t, clingo_literal_t, clingo_literal_t>> atoms_list_for_mapping;
     
    while (true) {
        bool equal;
        clingo_literal_t plit;
        clingo_literal_t slit;
        clingo_symbol_t symbol;
 
        // stop iteration if the end is reached
        handle_error(clingo_symbolic_atoms_iterator_is_equal_to(symbolic_atoms, symbolic_atoms_it, symbolic_atoms_ie, &equal));
        if (equal) { break; }
        handle_error(clingo_symbolic_atoms_symbol(symbolic_atoms, symbolic_atoms_it, &symbol));
        std::string symbol_str = from_symbol_to_string(symbol) ;

        
        handle_error(clingo_symbolic_atoms_literal(symbolic_atoms, symbolic_atoms_it, &plit));
        handle_error(clingo_propagate_init_solver_literal(init, plit, &slit));

        if (plit > max_plit) { max_plit = plit; }

        // debug("[init] symbol: ", symbol_str, " symbol: ", symbol, " plit: ", plit, " slit: ", slit)
        atoms_list_for_mapping.emplace_back(symbol, plit, slit);

      
        clingo_symbolic_atoms_next(symbolic_atoms, symbolic_atoms_it, &symbolic_atoms_it);
    }

    // debug("max plit: ", max_plit);

    for(const auto& [symbolic_atom, plit, slit]: atoms_list_for_mapping){
        this->atomNames.emplace(symbolic_atom, plit);
        map_plit_slit[plit] = slit ; 
        map_plit_slit[not_(plit)] = not_(slit) ; 
        update_map_value_vector<clingo_literal_t, clingo_literal_t>(map_slit_plit, slit, plit);
        update_map_value_vector<clingo_literal_t, clingo_literal_t>(map_slit_plit, not_(slit), not_(plit));
    }


    // debug("atomNames: ", atomNames_to_string(atomNames));

    AmoSumPropagator* propagator = new AmoSumPropagator(atomNames, param, propagation_phase, ge, choice_cons, solver = AmoSumPropagator::CLINGO);
    for (size_t i = 0; i < nt; i++) this->propagators.push_back(propagator);

    std::vector<clingo_literal_t> lits = {max_plit};

    std::vector<clingo_literal_t> facts = get_map_value_vector<clingo_literal_t, clingo_literal_t>(map_slit_plit, 1);
    extend_vector(lits, facts);

    std::vector<clingo_literal_t> to_watch_plit;
    auto start = std::chrono::high_resolution_clock::now();
    for (size_t i = 0; i < nt; i++) to_watch_plit = this->propagators[i]->getLiterals(lits) ;
    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed = end - start;
    debugf("getLiterals time: ",elapsed.count(), "s");

    // debug(vector_lit_to_string(atomNames, to_watch_plit, "to watch: "))
    size_t max_clause_size = this->propagators[0]->N;
    clause_clingo = new clingo_literal_t[max_clause_size];

    for(clingo_literal_t plit: to_watch_plit){
        clingo_literal_t slit = map_plit_slit[plit];
        update_map_value_vector(map_slit_plit_watched, slit, plit);
        handle_error(clingo_propagate_init_add_watch(init, slit));
    }


    std::vector<clingo_literal_t> S_plit ;
    for (size_t i = 0; i < nt; i++) S_plit = propagators[i]->simplifyAtLevelZero(true);
    
    if (S_plit.size() == 1 and S_plit[0] == BOTTOM){ 
        bool result ; 
        handle_error(clingo_propagate_init_add_clause((clingo_propagate_init*) init, NULL, 0, &result));
        return true; 
    }// inconsistent

    add_clauses_propagated_lits(init, S_plit, 0, true);
    return true ;
}

bool PropagatorClingo::add_clauses_propagated_lits(void *control, const std::vector<clingo_literal_t>& S_plit, int dl, bool init=false){
    
    int td ;
    init ? td = 0 : td = clingo_propagate_control_thread_id((clingo_propagate_control*) control); 

    
    AmoSumPropagator* prop = propagators[td];
    // dl = 0 ; // debugging
    for(clingo_literal_t plit: S_plit){
        const std::vector<clingo_literal_t>* R_plit = dl > 0 ? prop->getReasonForLiteral(plit) : nullptr;
        size_t clause_size = (dl > 0 ? R_plit->size() : 0) + 1 ;
        clingo_literal_t* clause = clause_clingo;
        // clingo_literal_t* clause = new clingo_literal_t[clause_size];
        clingo_literal_t slit = map_plit_slit[plit];
        clause[0] = slit ; 
        for (size_t i = 1; i < clause_size; i++) {
            clingo_literal_t r_plit =  (*R_plit)[i-1];
            clause[i] = map_plit_slit[r_plit];
        }


        // const clingo_assignment_t *assignment = clingo_propagate_init_assignment((clingo_propagate_init*) control);
        // clingo_truth_value_t truth_value;

        // // Get the truth value of the literal
        // if (!clingo_assignment_truth_value(assignment, slit, &truth_value)) {
        //     fprintf(stderr, "Error: Failed to get truth value of literal.\n");
        //     return true ;
        // }

        // // Check if the literal is already true
        // if (truth_value == clingo_truth_value_true) {
        //     printf("Literal %d is already true; no need to add a unit clause.\n", slit);
        //     return true;
        // }

        bool result_add_clause;
        init ? handle_error(clingo_propagate_init_add_clause((clingo_propagate_init*) control, clause, clause_size, &result_add_clause)) :
        handle_error(clingo_propagate_control_add_clause((clingo_propagate_control*) control, clause, clause_size, clingo_clause_type_learnt, &result_add_clause)) ;
        // debug("result_add_clause: ",result_add_clause, " plit: ", plit, " slit: ", slit, " name: ", get_name(atomNames, plit));
        
        // if(R_plit != nullptr) delete R_plit ; // it is handled internally

        // propagation must return immediately, there is a conflict
        if (not result_add_clause) return true ;

        bool result_propagate;
        init ? clingo_propagate_init_propagate((clingo_propagate_init*) control, &result_propagate) :
        clingo_propagate_control_propagate((clingo_propagate_control*) control, &result_propagate) ;
        
        // propagation must return immediately, a conflict has been raised 
        if (not result_propagate) return true;
        
    }
    return false ;
}   

bool PropagatorClingo::propagate(clingo_propagate_control_t *control, const clingo_literal_t *changes, size_t size){
    const clingo_assignment_t *assignment = clingo_propagate_control_assignment(control);
    int dl = clingo_assignment_decision_level(assignment);
    int td; 
    dl == 0 ? td = 0 : td = clingo_propagate_control_thread_id(control) ; 
    AmoSumPropagator* prop = propagators[td];

    print_propagate(this, changes, size, control, dl, false, false);

    for (size_t i = 0; i < size; i++)
    {
        clingo_literal_t slit = changes[i];
        std::vector<clingo_literal_t> plit_list = map_slit_plit_watched[slit];
        for(clingo_literal_t plit: plit_list){
            const std::vector<clingo_literal_t>* S_plit = prop->onLiteralTrue(plit, dl); // handled internally 
            if (S_plit != nullptr && add_clauses_propagated_lits(control, *S_plit, dl, false)){
                // Conflict added hence propagation has to stop
                return true;
            }
                  
        }
    }
    return true;
}



std::string PropagatorClingo::compute_changes_str(const clingo_literal_t *changes, size_t size, int td){
    std::vector<std::string> changes_name_vec ; 
    for (size_t i = 0; i < size; i++)
    {
        clingo_literal_t slit = changes[i] ;
        for(clingo_literal_t plit: map_slit_plit_watched[slit]){
            std::string name = get_name(atomNames, plit);
            std::string res = "[" + name + ", plit: " + std::to_string(plit) + ", slit: " +  std::to_string(slit) + "]";
            changes_name_vec.push_back(res);
        }
    }

    return vector_to_string(changes_name_vec,"");
}




void PropagatorClingo::undo(clingo_propagate_control_t *control, const clingo_literal_t *changes, size_t size){
   
    const clingo_assignment_t *assignment = clingo_propagate_control_assignment(control);
    int dl = clingo_assignment_decision_level(assignment);
    int td; 
    dl == 0 ? td = 0 : td = clingo_propagate_control_thread_id(control) ; 
    AmoSumPropagator* prop = propagators[td];

    std::vector<clingo_literal_t> plit_list;

    for (size_t i = 0; i < size; i++)
    {
        clingo_literal_t slit = changes[i];
        extend_vector(plit_list, map_slit_plit_watched[slit]);
    }
    print_undo(this, changes, size, control, dl, td, false, false);
    prop->onLiteralsUndefined(plit_list, false);
}




 


