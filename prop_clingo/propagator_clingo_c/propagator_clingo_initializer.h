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
#include "propagator_clingo.h"

class PropagatorClingoInitializer{

private:
    PropagatorClingoInitializer(){};
    bool first = true ;
    static PropagatorClingoInitializer* instance;

public:
    std::unordered_map<clingo_symbol_t, clingo_literal_t>* atomNames;
    std::unique_ptr<std::unordered_map<clingo_literal_t, std::vector<clingo_literal_t>>> map_slit_plit ;
    std::unique_ptr<std::unordered_map<clingo_literal_t, clingo_literal_t>> map_plit_slit ;
    size_t nt;
    clingo_literal_t max_plit ;
    std::vector<clingo_literal_t>* lits;


    static PropagatorClingoInitializer* get_instance(){
        if(instance == nullptr) instance = new PropagatorClingoInitializer() ;
        return instance ;
    }
    

    void init(clingo_propagate_init* _init, PropagatorClingo& propagator);

    ~PropagatorClingoInitializer(){
        if(lits) delete lits ;
        if(atomNames) delete atomNames;
    }

    static void cleanup(){
        delete instance ;
    }

};
