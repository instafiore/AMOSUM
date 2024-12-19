#pragma once
#include <iostream>
#include <clingo.h>
#include "utility.h"
#include <vector>

class AmoSumPropagator
{
private:
    /* data */
public:
    static constexpr const char* CLINGO = "clingo";
    static constexpr const char* WASP = "wasp";
    AmoSumPropagator(){}
    AmoSumPropagator(std::unordered_map<std::string, int> atomsNames,
                    std::unordered_map<std::string, std::string> params,
                    const std::vector<clingo_literal_t>* (*propagation_phase)(const Group&, AmoSumPropagator&) = NULL,
                    bool ge = true,
                    std::string choice_cons = "AMO",
                    std::string SOLVER = "WASP"
                    ){}
    ~AmoSumPropagator();

    bool mps_violated = false ; // whether the mps is violated

    std::vector<clingo_literal_t>* reason ; // common reason for literals 
    PerfectHash<std::vector<clingo_literal_t>*>* reason_trues;  // reason for true_literals
    std::vector<Group> groups; // # a list of Group
    TrueGroupFunction* true_group;  //  a function from self.groups -> literals U {0}; if true_group[G] = 0 then there is not true literal in G

    std::vector<clingo_literal_t> getLiterals(const std::vector<clingo_literal_t>& lits){return std::vector<clingo_literal_t>();}
};


