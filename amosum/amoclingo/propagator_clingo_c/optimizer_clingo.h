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

class OptimizerClingo{

private:
    // static OptimizerClingo* instance;
    
public:
    OptimizerClingo(const ParameterMap &params,PropagatorClingo* propagator): propagator(propagator), params(params){}
    PropagatorClingo* propagator;
    const ParameterMap &params;
    std::vector<AnswerSet*> answerSets;
    AnswerSet* currentAnswerSet = nullptr;

    // static OptimizerClingo* getInstance();
    // static void initInstace(const ParameterMap &params,PropagatorClingo* propagator);
    

    bool init(clingo_propagate_init_t *_init);
    virtual bool check(clingo_propagate_control_t *control) noexcept;

    std::vector<AmoSumPropagator*> propagators ; 
    ~OptimizerClingo(){
        if(currentAnswerSet != nullptr) {delete currentAnswerSet; currentAnswerSet = nullptr; }
        for(AnswerSet* ans: answerSets) if(ans != nullptr) {delete ans; ans = nullptr; }
    }

    // static void cleanup(){
    //     delete instance ;
    // }

};