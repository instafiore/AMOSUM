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
#include "optimizer_clingo.h"
#include "../../amosum_initializer.h"

class OptimizerProgressiveClingo: public OptimizerClingo{


    OptimizerProgressiveClingo(const ParameterMap &params,PropagatorClingo* propagator): OptimizerClingo(params, propagator){}
    bool check(clingo_propagate_control_t *control) noexcept override;

};