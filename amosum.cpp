#include "amosum.h"
#include <iostream>


std::vector<clingo_literal_t> AmoSumPropagator::getLiterals(const std::vector<clingo_literal_t>& lits){
        N = lits[0] + 1;
        
        // TODO: remove
        std::vector<clingo_literal_t> to_watch;
        extend_vector(to_watch, lits, 1);
        // 
        
        minimization = get_map(params, std::string("min_r"), std::string(Minimize::NO_MINIMIZATION)) ;
        strategy = get_map(params, std::string("strategy"), strategy);
        N = lits[0] + 1;
        I.reset(new InterpretationFunction(N));
        weight.reset(new WeightFunction(N));
        group.reset(new GroupFunction(N));
        aggregate.reset(new AggregateFunction(N));
        reason_trues.reset(new PerfectHash<vector_lit_ptr> (N, nullptr));
        redundant_lits.reset(new PerfectHash<vector_lit_ptr> (N, nullptr));
        _mps = 0;
        ID = get_map(params, std::string("id"), std::string("0"));
        groups.clear();  // Initialize as empty
        assumptions = get_map(params, std::string("ass"), std::string("false"));
        current_sum = 0;
        lazy_prop_actived = get_map(params, std::string("lazy"), std::string("false")) == "true";
        lazy_condition = !lazy_prop_actived;
 
        return to_watch;
}
