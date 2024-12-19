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
    std::unordered_map<std::string, std::string> sys_parameters;
    const std::vector<clingo_literal_t>* (*propagation_phase)(const Group&, AmoSumPropagator&);
    bool ge;
    std::string prop_type;
    std::string solver;
    PropagatorClingo(
            const std::unordered_map<std::string, std::string>& sys_parameters,
            const std::vector<clingo_literal_t>* (*propagation_phase)(const Group&, AmoSumPropagator&),
            bool ge,
            const std::string& prop_type
        )
            : sys_parameters(sys_parameters),
            propagation_phase(propagation_phase),
            ge(ge),
            prop_type(prop_type),
            solver(AmoSumPropagator::CLINGO) {}

    bool init(clingo_propagate_init_t *init);
    bool propagate(clingo_propagate_control_t *control, const clingo_literal_t *changes, size_t size, AmoSumPropagator *propagator){return true;}
    void undo(clingo_propagate_control_t *control, const clingo_literal_t *changes, size_t size, AmoSumPropagator *propagator){}

};