
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

    AmoSumPropagator* propagator = new AmoSumPropagator(atomNames, param, propagation_phase, ge, choice_cons, solver = AmoSumPropagator::CLINGO);
    for (size_t i = 0; i < PropagatorClingoInitializer::get_instance()->nt; i++) this->propagators.push_back(propagator);

    std::vector<clingo_literal_t> to_watch_plit;

    for (size_t i = 0; i < PropagatorClingoInitializer::get_instance()->nt; i++) to_watch_plit = AmoSumInitializer::get_instance()->getLiterals(*PropagatorClingoInitializer::get_instance()->lits, this->propagators[i]) ;
    // for (size_t i = 0; i < PropagatorClingoInitializer::get_instance()->nt; i++) to_watch_plit = this->propagators[i]->getLiterals(*PropagatorClingoInitializer::get_instance()->lits) ;

    size_t max_clause_size = this->propagators[0]->N;
    // debugf("max_clause_size: ",max_clause_size);
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
    // dl = 0 ; // debugging
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

        // if(slit == 6440) {
        //     for(auto rplit: *R_plit){
        //         assert(!prop->I->get(rplit));
        //     }
        //     assert(prop->I->get(plit) == SETTINGS::NONE);
        //     clingo_truth_value_t value;
        //     size_t unassigned_count = 0;
        //     const clingo_assignment_t *assignment = clingo_propagate_control_assignment((clingo_propagate_control*)control);
        //     for (size_t i = 0; i < clause_size; ++i) {
        //         clingo_literal_t literal = clause[i];
        //         clingo_assignment_truth_value(assignment, literal, &value);

        //         if (value == clingo_truth_value_free) {
        //             ++unassigned_count;
        //         }
        //     }
        //     clingo_assignment_truth_value(assignment, slit, &value);
        //     if(value == clingo_truth_value_free) debugf("slit is undef");
        //     if(value == clingo_truth_value_true) debugf("slit is true");
        //     if(value == clingo_truth_value_false) debugf("slit is false");
        //     debugf("unassigned_count: ", unassigned_count);
        //     // assert(value == clingo_truth_value_free);
        //     // assert(unassigned_count == 1);
            
        //     debugf(" propagated")
        // };


        bool result_add_clause;
        init ? handle_error(clingo_propagate_init_add_clause((clingo_propagate_init*) control, clause, clause_size, &result_add_clause)) :
        handle_error(clingo_propagate_control_add_clause((clingo_propagate_control*) control, clause, clause_size, clingo_clause_type_learnt, &result_add_clause)) ;

        // propagation must return immediately, there is a conflict
        if (not result_add_clause){ 
            // for(int sj = si ; sj < S_plit.size(); ++sj){
            //     clingo_literal_t plit_not_propagated = S_plit[sj];
            //     prop->to_be_propagated->set(plit_not_propagated, false);
            // }
            // debugf("conflict add clause");
            return true ;
        }

        // if(slit == 10163){
        //     const clingo_assignment_t *assignment = clingo_propagate_control_assignment((clingo_propagate_control*)control);
        //     clingo_truth_value_t value;
        //     clingo_assignment_truth_value(assignment, slit, &value);
        //     if(value == clingo_truth_value_free) debugf("slit is undef");
        //     if(value == clingo_truth_value_true) debugf("slit is true");
        //     if(value == clingo_truth_value_false) debugf("slit is false");
        //     assert(value == clingo_truth_value_true);
        // }
    }
    return false ;
}   

bool PropagatorClingo::propagate(clingo_propagate_control_t *control, const clingo_literal_t *changes, size_t size){

    const clingo_assignment_t *assignment = clingo_propagate_control_assignment(control);
    int dl = clingo_assignment_decision_level(assignment);
    int td; 
    dl == 0 ? td = 0 : td = clingo_propagate_control_thread_id(control) ; 
    AmoSumPropagator* prop = propagators[td];
    // print_propagate(this, changes, size, control, dl, dl >= 5000 , false);

    
    std::vector<clingo_literal_t> to_propagate;
    for (size_t i = 0; i < size; i++)
    {
        clingo_literal_t slit = changes[i];
        std::vector<clingo_literal_t> plit_list = map_slit_plit_watched[slit];
        
        for(clingo_literal_t plit: plit_list){
            const std::vector<clingo_literal_t>* S_plit = prop->onLiteralTrue(plit, dl); // handled internally 
            if(S_plit != nullptr) extend_vector(to_propagate, *S_plit);
            if (S_plit != nullptr && add_clauses_propagated_lits(control, *S_plit, dl, false)){
                // for (size_t j = i; j < size; j++)
                // {
                //     clingo_literal_t slit_not_prop = changes[j];
                //     prop->to_be_propagated->set(slit_not_prop, false);
                // }
                for(auto split: to_propagate){
                    prop->to_be_propagated->set(split, false);
                }
                // debugf("conflict");
                return true;
            }
                  
        }
    }
    bool result_propagate;
    clingo_propagate_control_propagate(control, &result_propagate) ;
    

    // propagation must return immediately, a conflict has been raised 
    for(auto split: to_propagate){
        prop->to_be_propagated->set(split, false);
    }
    if (not result_propagate){ 
        // debugf("conflict propagate");
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
    // if(dl >= 5000){
    //     clingo_literal_t slit = 6440 ;
    //     const clingo_assignment_t *assignment = clingo_propagate_control_assignment((clingo_propagate_control*)control);
    //     clingo_truth_value_t value;
    //     clingo_assignment_truth_value(assignment, slit, &value);
    //     if(value == clingo_truth_value_free) debugf("slit is undef");
    //     if(value == clingo_truth_value_true) debugf("slit is true");
    //     if(value == clingo_truth_value_false) debugf("slit is false");
    // }
    // print_undo(this, changes, size, control, dl, td, dl >= 5000, false);
    prop->onLiteralsUndefined(plit_list, false);
}




 


