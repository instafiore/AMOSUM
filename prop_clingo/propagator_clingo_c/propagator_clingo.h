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

class PropagatorClingo{

private:
    std::unordered_map<std::string, clingo_literal_t> atomNames;

public:
    bool init(clingo_propagate_init_t *init);

    bool propagate(clingo_propagate_control_t *control, const clingo_literal_t *changes, size_t size, AmoSumPropagator *propagator){return true;}
    void undo(clingo_propagate_control_t *control, const clingo_literal_t *changes, size_t size, AmoSumPropagator *propagator){}

};