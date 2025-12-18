#pragma once
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
#include <memory>
#include "propagator_clingo_initializer.h"
#include "../../amosum_initializer.h"

class PropagatorClingo{

private:
public:
    std::unordered_map<clingo_symbol_t, clingo_literal_t>* atomNames;
    
    std::unordered_map<std::string, std::string> param;
    const std::vector<clingo_literal_t>* (*propagation_phase)(const Group*, AmoSumPropagator*);
    bool ge;
    std::string choice_cons;
    std::string solver;
    int bound ;
    bool first = true;
    bool firstPropagate = true;
    bool maximizer = false ;
    std::vector<clingo_literal_t> to_watch_plit;
    // std::vector<clingo_literal_t> to_watch_slit;

    // void removeWatches(clingo_control_t *ctl);
    

    // This is a map for mapping each solver literal (slit) to its program literal(s) (plit).
    // Can happend that some solver literal has more than one program literal
    std::unordered_map<clingo_literal_t, std::vector<clingo_literal_t>>* map_slit_plit ;

    // This map maps each solver literal (watched) to its program literals (watched)
    std::unordered_map<clingo_literal_t, std::vector<clingo_literal_t>> map_slit_plit_watched ;

    // inverse of map_slit_plit
    std::unordered_map<clingo_literal_t, clingo_literal_t>* map_plit_slit = nullptr ;

    clingo_literal_t* clause_clingo ;

    std::vector<clingo_literal_t> true_plit;


    bool enabled;

    PropagatorClingo(
            const std::unordered_map<std::string, std::string>& param,
            const std::vector<clingo_literal_t>* (*propagation_phase)(const Group*, AmoSumPropagator*),
            bool ge,
            const std::string& choice_cons,
            bool maximizer = false
        )
            : param(param),
            propagation_phase(propagation_phase),
            ge(ge),
            choice_cons(choice_cons),
            solver(AmoSumPropagator::CLINGO),
            maximizer(maximizer),
            bound(SETTINGS::NONE),
            enabled(true)
            {}

    
    
    void updateBound(int bound, size_t td);
    void reset();
    
    bool init(clingo_propagate_init_t *_init);
    bool propagate(clingo_propagate_control_t *control, const clingo_literal_t *changes, size_t size);
    void undo(clingo_propagate_control_t *control, const clingo_literal_t *changes, size_t size);

    bool add_clauses_propagated_lits(void *control, const std::vector<clingo_literal_t>& S_plit, int dl, bool init);
    std::string compute_changes_str(const clingo_literal_t *changes, size_t size, int td);

    std::unordered_map<std::string,int>&  weights_names();

    std::vector<AmoSumPropagator*> propagators ; 
    ~PropagatorClingo(){
        for(auto& prop: propagators){
            if(prop) delete prop;
        }
        propagators.clear();
        if(clause_clingo != nullptr) delete[] clause_clingo;
    }
};