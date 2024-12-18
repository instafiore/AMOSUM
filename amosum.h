#include <iostream>
#include <clingo.h>
#include "utility.h"


class AmoSumPropagator
{
private:
    /* data */
public:
    AmoSumPropagator(){}
    AmoSumPropagator(std::unordered_map<std::string, int> atomsNames,
                    std::vector<std::string> sys_parameters, 
                    const std::vector<clingo_literal_t>& (*propagation_phase)(Group, AmoSumPropagator) = NULL,
                    bool ge = true,
                    std::string choice_cons = "AMO",
                    std::string SOLVER = "WASP"
                    ){}
    ~AmoSumPropagator();
};


