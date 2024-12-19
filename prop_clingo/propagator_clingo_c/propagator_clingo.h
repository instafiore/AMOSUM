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
    std::unordered_map<std::string, std::string> param;
    const std::vector<clingo_literal_t>* (*propagation_phase)(const Group&, AmoSumPropagator&);
    bool ge;
    std::string choice_cons;
    std::string solver;
    std::vector<AmoSumPropagator*> propagators ; 

    // This is a map for mapping each solver literal (slit) to its program literal(s) (plit).
    // Can happend that some solver literal has more than one program literal
    std::unordered_map<clingo_literal_t, std::vector<clingo_literal_t>> map_slit_plit ;

    // This map maps each solver literal (watched) to its program literals (watched)
    std::unordered_map<clingo_literal_t, std::vector<clingo_literal_t>> map_slit_plit_watched ;

    // inverse of map_slit_plit
    std::unordered_map<clingo_literal_t, clingo_literal_t> map_plit_slit ;

    PropagatorClingo(
            const std::unordered_map<std::string, std::string>& param,
            const std::vector<clingo_literal_t>* (*propagation_phase)(const Group&, AmoSumPropagator&),
            bool ge,
            const std::string& choice_cons
        )
            : param(param),
            propagation_phase(propagation_phase),
            ge(ge),
            choice_cons(choice_cons),
            solver(AmoSumPropagator::CLINGO) {}

    bool init(clingo_propagate_init_t *init);
    bool propagate(clingo_propagate_control_t *control, const clingo_literal_t *changes, size_t size, AmoSumPropagator *propagator){return true;}
    void undo(clingo_propagate_control_t *control, const clingo_literal_t *changes, size_t size, AmoSumPropagator *propagator){}

    ~PropagatorClingo(){
        for(auto& prop: propagators){
            delete prop;
        }
    }
};