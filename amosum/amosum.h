#pragma once
#include <iostream>
#include <clingo.h>
#include "utility.h"
#include <vector>
#include <memory>
#include <unordered_set>
using vector_lit_ptr = std::vector<clingo_literal_t>* ;

struct AmoSumPropagator
{
    std::unordered_map<clingo_symbol_t, clingo_literal_t>* atomNames;

    std::unordered_map<std::string, std::string> params; // Stores additional parameters

    std::string ID;// aggregate id

    size_t N ; // number of atoms in the amo sum aggregate

    std::unique_ptr<InterpretationFunction> I ; // A function from literals -> {true, false, NONE}

    // A function from literals -> weights
    // assuming that the aggregate does not contain the to literal l and !l
    WeightFunction* weight ;

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

    // Set of derived lits
    std::vector<clingo_literal_t> S ;

    // reason common to all literals (either true or false literal)
    std::unique_ptr<PerfectHash<std::vector<clingo_literal_t>*>> reason  ;

    // Redundant literals in reason of a literal l
    // it is a funtion lits -> 2^(lits)
    std::unique_ptr<PerfectHash<std::unordered_set<clingo_literal_t>*>> redundant_lits ;
    

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
    const std::vector<clingo_literal_t>* (*propagation_phase)(const Group*, AmoSumPropagator*); // Function pointer for propagation
    
    // treshold for lazy propagation activation
    float lazy_perc = SETTINGS::NONE ;

    // whether the mps is violated
    bool mps_violated = false ; 

    int current_sum ;

    bool lazy_prop_activated = false;
    bool lazy_condition ;

    // whether is inconsistent or not at level 0
    bool inconsistent_at_level_0 ;
    std::vector<clingo_literal_t> groups_literals ;

    clingo_propagate_control_t *control = nullptr;
    // inverse of map_slit_plit
    std::unordered_map<clingo_literal_t, clingo_literal_t>* map_plit_slit ;

    int lb = SETTINGS::NONE;      // lower bound
    int _mps;    // max/min possible sum
    int ub = SETTINGS::NONE;      // upper bound
    int bound = SETTINGS::NONE ; // either lb or ub depending on ge
    
    std::string solver; 
    static constexpr const char* CLINGO = "clingo";
    static constexpr const char* WASP = "wasp";

    std::unordered_map<std::string,int> weights_names ;

    unsigned long count = 0 ;
    clingo_literal_t current_literal;

    void updateBound(int bound);

    bool maximizer;

    
    AmoSumPropagator(){}
    AmoSumPropagator(
        std::unordered_map<clingo_symbol_t, clingo_literal_t>* atomNames,
        std::unordered_map<std::string, std::string> params,
        const std::vector<clingo_literal_t>* (*propagation_phase)(const Group*, AmoSumPropagator*) = nullptr,
        bool ge = true,
        std::string choice_cons = "AMO",
        std::string solver = "WASP",
        bool maximizer = false )
        : atomNames(atomNames),
          params(std::move(params)),
          propagation_phase(propagation_phase),
          ge(ge),
          choice_cons(std::move(choice_cons)),
          solver(std::move(solver)),
          maximizer(maximizer)
          {}

    ~AmoSumPropagator(){
        for(const Group* group : groups)   delete group ;
        for(int i=0; i< redundant_lits->N; ++i){
            auto ptr_set = redundant_lits->get(i);
            if (ptr_set != nullptr) delete ptr_set ;
            auto ptr_vec = reason->get(i);
            if (ptr_vec != nullptr) delete ptr_vec ;
        }
    }

    void resetPropagator();
    const std::vector<clingo_literal_t> getLiterals(const std::vector<clingo_literal_t>& lits);
    const std::vector<clingo_literal_t> simplifyAtLevelZero(const bool& delete_lits);
    const std::vector<clingo_literal_t>* getReasonForLiteral(const clingo_literal_t& lit);
    const std::vector<clingo_literal_t>* onLiteralTrue(const clingo_literal_t& lit, const int& dl);
    void onLiteralsUndefined(const std::vector<clingo_literal_t> &plit_list, bool wasp);


    void update_lazy_propagation();
    std::pair<bool, Group*> update_phase(clingo_literal_t l, int dl);
    std::tuple<int, clingo_literal_t, clingo_literal_t> mps(Group* g, clingo_literal_t l, bool assumed);
    void updated_dl(int lit, int new_dl);
    inline bool is_in_aggregate(clingo_literal_t lit){ return aggregate->get(lit) || aggregate->get(not_(lit));}
    void compute_minimal_reason(const std::vector<clingo_literal_t>& to_minimize);

    void add_redundant_lits(clingo_literal_t l, std::vector<clingo_literal_t> redundant_lits_vec);
    void add_redundant_lit(clingo_literal_t l, clingo_literal_t redundant_l);
    bool is_true(clingo_literal_t l);
    bool is_false(clingo_literal_t l);
    bool is_undef(clingo_literal_t l);
};


