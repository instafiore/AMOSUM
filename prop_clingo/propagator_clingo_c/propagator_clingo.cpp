
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


bool PropagatorClingo::init(clingo_propagate_init_t *_init){

    PropagatorClingoInitializer::get_instance()->init(_init, *this);

    for (size_t i = 0; i < PropagatorClingoInitializer::get_instance()->nt; i++){
        AmoSumPropagator* propagator = new AmoSumPropagator(atomNames, param, propagation_phase, ge, choice_cons, solver = AmoSumPropagator::CLINGO);
        propagator->map_plit_slit = map_plit_slit ;        
        this->propagators.push_back(propagator);
    }
    std::vector<clingo_literal_t> to_watch_plit;

    for (size_t i = 0; i < PropagatorClingoInitializer::get_instance()->nt; i++) to_watch_plit = AmoSumInitializer::get_instance()->getLiterals(*PropagatorClingoInitializer::get_instance()->lits, this->propagators[i]) ;
    
    size_t max_clause_size = this->propagators[0]->N;
    clause_clingo = new clingo_literal_t[max_clause_size];

    for(clingo_literal_t plit: to_watch_plit){
        clingo_literal_t slit = (*map_plit_slit)[plit];
        update_map_value_vector(map_slit_plit_watched, slit, plit);
        handle_error(clingo_propagate_init_add_watch(_init, slit));
    }


    std::vector<clingo_literal_t> S_plit ;
    for (size_t i = 0; i < PropagatorClingoInitializer::get_instance()->nt; i++) S_plit = propagators[i]->simplifyAtLevelZero(true);
    
    if (S_plit.size() == 1 and S_plit[0] == BOTTOM){ 
        bool result ; 
        handle_error(clingo_propagate_init_add_clause((clingo_propagate_init*) _init, NULL, 0, &result));
        return true; 
    }// inconsistent

    add_clauses_propagated_lits(_init, S_plit, 0, true);
    return true ;
}

bool PropagatorClingo::add_clauses_propagated_lits(void *control, const std::vector<clingo_literal_t>& S_plit, int dl, bool init=false){
    
    int td ;
    init ? td = 0 : td = clingo_propagate_control_thread_id((clingo_propagate_control*) control); 

    
    AmoSumPropagator* prop = propagators[td];
    for(int si = 0 ; si < S_plit.size();  ++si){
        clingo_literal_t plit = S_plit[si];
        const std::vector<clingo_literal_t>* R_plit = dl > 0 ? prop->getReasonForLiteral(plit) : nullptr;
        size_t clause_size = (dl > 0 ? R_plit->size() : 0) + 1 ;
        clingo_literal_t* clause = clause_clingo;
        clingo_literal_t slit = (*map_plit_slit)[plit];
        clause[0] = slit ; 
        assert(clause_size <= this->propagators[0]->N);
        for (size_t i = 1; i < clause_size; i++) {
            clingo_literal_t r_plit =  (*R_plit)[i-1];
            clause[i] = (*map_plit_slit)[r_plit];
        }

        bool result_add_clause;
        init ? handle_error(clingo_propagate_init_add_clause((clingo_propagate_init*) control, clause, clause_size, &result_add_clause)) :
        handle_error(clingo_propagate_control_add_clause((clingo_propagate_control*) control, clause, clause_size, clingo_clause_type_learnt, &result_add_clause)) ;

        // propagation must return immediately, there is a conflict
        if (not result_add_clause){
            // debugf("conflict add clause");
            return true ;
        }


        bool result_propagate;
        init ? handle_error(clingo_propagate_init_propagate((clingo_propagate_init*) control, &result_propagate)) :
        handle_error(clingo_propagate_control_propagate((clingo_propagate_control*)control, &result_propagate)) ;
        
        if (!result_propagate){ 
            // propagation must return immediately, a conflict has been raised 
            return true ;
        }   
    }
    return false ;
}   

bool PropagatorClingo::propagate(clingo_propagate_control_t *control, const clingo_literal_t *changes, size_t size){
    const clingo_assignment_t *assignment = clingo_propagate_control_assignment(control);
    int dl = clingo_assignment_decision_level(assignment);
    int td; 
    dl == 0 ? td = 0 : td = clingo_propagate_control_thread_id(control) ; 
    AmoSumPropagator* prop = propagators[td];
    prop->control = control ;
    print_propagate(this, changes, size, control, dl, false, false);


    for (size_t i = 0; i < size; i++)
    {
        clingo_literal_t slit = changes[i];
        std::vector<clingo_literal_t> plit_list = map_slit_plit_watched[slit];
        
        for(clingo_literal_t plit: plit_list){
            const std::vector<clingo_literal_t>* S_plit = prop->onLiteralTrue(plit, dl); // handled internally 
            if (S_plit != nullptr && add_clauses_propagated_lits(control, *S_plit, dl, false)){ 
                // debugf("conflict");
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
            assert(atomNames != NULL);
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




 


