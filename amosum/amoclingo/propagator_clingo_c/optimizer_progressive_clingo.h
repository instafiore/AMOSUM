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
#include "propagator_clingo.h"
#include "../../amosum_initializer.h"

class OptimizerProgressiveClingo{

private:
    static OptimizerProgressiveClingo* instance;
    OptimizerProgressiveClingo(const ParameterMap &params,PropagatorClingo* propagator): propagator(propagator), params(params){}

public:
    PropagatorClingo* propagator;
    const ParameterMap &params;
    AnswerSet* currentAnswerSet = nullptr;

    static OptimizerProgressiveClingo* getInstance();
    static void initInstace(const ParameterMap &params,PropagatorClingo* propagator);
    

    bool init(clingo_propagate_init_t *_init);
    bool check(clingo_propagate_control_t *control);

    void discardCurrentCost(clingo_propagate_control_t *control, size_t td);

    std::vector<AmoSumPropagator*> propagators ; 
    ~OptimizerProgressiveClingo(){
        if(currentAnswerSet != nullptr) delete currentAnswerSet;
    }

    static void cleanup(){
        delete instance ;
    }

};