#pragma once
#include "amosum.h"

struct generic_data{
    int bound = SETTINGS::NONE;
    std::map<std::string, std::vector<clingo_literal_t>> groups_raw;
    std::vector<clingo_literal_t> bind;
    std::unordered_map<std::string,int> weights_names ;
};

class AmoSumInitializer{

private:
    AmoSumInitializer(){};
    bool first = true ;
    static AmoSumInitializer* instance;
    std::unordered_map<std::string, clingo_literal_t> atomNamesString;
    std::unique_ptr<WeightFunction> weight ;
    std::unordered_map<std::string, AggregateFunction*> aggregate_map ;
    std::unordered_map<std::string, generic_data*> generic_data_map ;

public:
    static AmoSumInitializer* get_instance(){
        if(instance == nullptr) instance = new AmoSumInitializer() ;
        return instance ;
    }

    const std::vector<clingo_literal_t> getLiterals(const std::vector<clingo_literal_t>& lits, AmoSumPropagator* amosum_propagator);
    void common_phase(AmoSumPropagator* amosum_propagator);
    void assign(AmoSumPropagator* amosum_propagator);
    void specific_phase(const std::vector<clingo_literal_t>& lits, AmoSumPropagator* amosum_propagator);

    ~AmoSumInitializer(){
        for(auto& [key, value]: generic_data_map){
            if(value) delete value;
        }
    }

    static void cleanup(){
        delete instance ;
    }
};