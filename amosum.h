#pragma once
#include <iostream>
#include <clingo.h>
#include "utility.h"
#include <vector>
#include <memory>
using vector_lit_ptr = std::vector<clingo_literal_t>* ;
struct AmoSumPropagator
{
    std::unordered_map<clingo_symbol_t, clingo_literal_t> atomsNames;

    std::unordered_map<std::string, std::string> params; // Stores additional parameters

    std::string ID;// aggregate id

    size_t N ; // number of atoms in the amo sum aggregate

    std::unique_ptr<InterpretationFunction> I ; // A function from literals -> {true, false, NONE}

    // A function from literals -> weights
    // assuming that the aggregate does not contain the to literal l and !l
    std::unique_ptr<WeightFunction> weight ;

    // A function from literals -> {True, False}
    std::unique_ptr<AggregateFunction> aggregate;

    // A function from literals -> groups
    std::unique_ptr<GroupFunction> group ;

    // A function from literals -> groups
    std::unique_ptr<TrueGroupFunction> true_group ;

    // A list of group
    std::vector<Group*> groups ;

    // Literals derived at level 0
    std::vector<clingo_literal_t> facts ;

    // Literals derived at level 0
    std::unique_ptr<std::vector<clingo_literal_t>> reason ;

    // A reason for true literals
    std::unique_ptr<PerfectHash<vector_lit_ptr>> reason_trues ;

    // Redundant literals in reason of a literal l
    // it is a funtion lits -> 2^(lits)
    std::unique_ptr<PerfectHash<vector_lit_ptr>> redundant_lits ;

    // assumptions as a list of atom names [json notation]
    std::string assumptions;

    // strategy with which to create the minimal reason, default is the order given in input
    std::string strategy ;

    // the last decision literal
    int last_decision_lit ;

    // type of minimization, default no minimization
    std::string minimization = Minimize::NO_MINIMIZATION ;

    // Defining the constraint type, possible values: AMO, EO
    std::string choice_cons; 

    // defining whether the propagator is for the constraint >=  (ge) or <= (le) 
    bool ge; 

    // decision level
    int dl = 0 ;

    // propagate function to implement in propagator file
    const std::vector<clingo_literal_t>* (*propagation_phase)(const Group&, AmoSumPropagator&); // Function pointer for propagation
    
    // treshold for lazy propagation activation
    float LAZY_PERC = 0.98 ;

    // whether the mps is violated
    bool mps_violated = false ; 

    int current_sum ;

    bool lazy_prop_actived = false;
    bool lazy_condition ;

    // whether is inconsistent or not at level 0
    bool inconsistent_at_level_0 ;

    std::vector<clingo_literal_t> groups_literals ;


    int lb;      // lower bound
    int _mps;    // max/min possible sum
    int ub;      // upper bound

    std::string solver; 
    static constexpr const char* CLINGO = "clingo";
    static constexpr const char* WASP = "wasp";

    AmoSumPropagator(){}
    AmoSumPropagator(
        std::unordered_map<clingo_symbol_t, clingo_literal_t> atomsNames,
        std::unordered_map<std::string, std::string> params,
        const std::vector<clingo_literal_t>* (*propagation_phase)(const Group&, AmoSumPropagator&) = nullptr,
        bool ge = true,
        std::string choice_cons = "AMO",
        std::string solver = "WASP")
        : atomsNames(std::move(atomsNames)),
          params(std::move(params)),
          propagation_phase(propagation_phase),
          ge(ge),
          choice_cons(std::move(choice_cons)),
          solver(std::move(solver)) {}
    ~AmoSumPropagator(){
        for(const Group* group : groups)   delete group ;
    }

    std::vector<clingo_literal_t> getLiterals(const std::vector<clingo_literal_t>& lits);
    std::vector<clingo_literal_t> simplifyAtLevelZero(bool delete_lits=false){ return std::vector<clingo_literal_t>(); };
    std::vector<clingo_literal_t>* getReasonForLiteral(clingo_literal_t lit){ return new std::vector<clingo_literal_t>(); }
    std::vector<clingo_literal_t>* onLiteralTrue(clingo_literal_t plit, int dl){ return new std::vector<clingo_literal_t>();}
    void onLiteralsUndefined(const std::vector<clingo_literal_t> &plit_list, bool wasp){}

};


