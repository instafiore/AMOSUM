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
#include "propagator_clingo.h"
#include <chrono>
#include <csignal>
#include <sys/signal.h>

// void signalHandler(int signum) {
//     print("Received SIGINT (Ctrl+C)");
//     exit(signum);
// }

PropagatorClingo* register_propagator(clingo_control_t *ctl, clingo_propagator_t prop, std::string prop_type, const ParameterMap& param, std::vector<PropagatorClingo*> &propagators);
bool init(clingo_propagate_init_t *_init, PropagatorClingo *propagator){
    bool res =  propagator->init(_init);
    return res;
}
bool propagate(clingo_propagate_control_t *control, const clingo_literal_t *changes, size_t size, PropagatorClingo *propagator){ 
    bool res =  propagator->propagate(control, changes, size);
    return res;
}
void undo(clingo_propagate_control_t *control, const clingo_literal_t *changes, size_t size, PropagatorClingo *propagator){
    propagator->undo(control, changes, size);
}

int main(int argc, char const *argv[])
{


    // signal(SIGINT, signalHandler);
    ParameterMap params =  init_param(argc, argv);

    std::string encoding_path = "" ;
    params.find("enc") != params.end() ? encoding_path = params.find("enc")->second : NULL ;
    std::string instance_path = "" ;
    params.find("i") != params.end() ? instance_path = params.find("i")->second : NULL ;

    int major, minor, revision;
    clingo_version(&major, &minor, &revision);
    // printf("Clingo version: %d.%d.%d\n", major, minor, revision);

    std::string encoding = cat(encoding_path);
    std::string instance = cat(instance_path);

    ParameterMap::iterator it_amosum = params.find("amosum_propagator")  ;

    std::vector<std::string> sys_parameters;
    if (it_amosum != params.end()){
        std::string sys_parameters_str = it_amosum->second ;
        sys_parameters = split(sys_parameters_str, ' ');
    }

    char const *error_message;
    int ret = 0;
    clingo_solve_result_bitset_t solve_ret;
    clingo_control_t *ctl = NULL;
    const char *args[] = {"--seed=42", "--parallel-mode=1"};
    handle_error(clingo_control_new(args, 2, NULL, NULL, 20, &ctl));
    clingo_part_t parts[] = {{"base", NULL, 0 }};


    clingo_configuration_t *config = NULL;
    clingo_id_t root_key, solve_key, seed_key;

    // create a propagator with the functions above
    // using the default implementation for the model check
    clingo_propagator_t prop = {
        (clingo_propagator_init_callback_t)init,
        (clingo_propagator_propagate_callback_t)propagate,
        (clingo_propagator_undo_callback_t)undo,
        NULL,
        NULL,
    };


    std::vector<std::pair<std::string, ParameterMap>> prop_type_params = process_sys_parameters(sys_parameters) ;

    std::vector<PropagatorClingo*> propagators ;
    PropagatorClingo* propagatorMaximize = nullptr ;
    for(auto& [prop_type, param]: prop_type_params){
        if (prop_type == "amomaximize"){
            propagatorMaximize = register_propagator(ctl, prop, "ge_amo", param, propagators);
            propagatorMaximize->maximizer = true;
        }else{
            register_propagator(ctl, prop, prop_type, param, propagators);
        }
    }
    
    // Loading the program to the control
    if (params.find("enc") != params.end()) handle_error(clingo_control_load(ctl, encoding_path.c_str())) ;
    if (params.find("i") != params.end()) handle_error(clingo_control_load(ctl, instance_path.c_str()));
    

    handle_error((clingo_control_ground(ctl, parts, 1, NULL, NULL)));

    int countMaximize = 0;
    int nMaximize = 10000;

    Result* resultOpt = nullptr ;
    Result* previous = nullptr ;

    // params["serialize"] = "True";
    int bound = 0;
    while(true){

        Result* result;
        
        handle_error(solve(ctl, result, propagatorMaximize));
        
        
        if(params.find("serialize") == params.end())  printf("%s\n",result->toString().c_str());
        else  printf("%s\n",result->serialize().c_str());

        // if(bound == 213628){
        //     printf("OH WOW\n");
        //     exit(0);
        // }

        if(propagatorMaximize == nullptr){
            resultOpt = result; 
            break;
        }

        if(result->exitCode != 10) break;
        if(resultOpt != nullptr) previous = resultOpt;
        resultOpt = result; 


        bound = result->model->cost + 1;
        propagatorMaximize->updateBound(bound);

        if(previous != nullptr) delete previous;   

        AmoSumInitializer::get_instance()->reset();
        PropagatorClingoInitializer::get_instance()->reset();
        propagatorMaximize->reset();

        propagatorMaximize->enabled = false;
        propagatorMaximize = new PropagatorClingo(*propagatorMaximize);
        propagatorMaximize->enabled = true;
        handle_error(clingo_control_register_propagator(ctl, &prop, propagatorMaximize, false));
        
        // ++countMaximize;
        // if(countMaximize >= nMaximize) break;
    }

    if(propagatorMaximize != nullptr) {
        resultOpt->setOptimum();
        if(params.find("serialize") == params.end())  printf("%s\n",resultOpt->toString().c_str());
        else  printf("%s\n",resultOpt->serialize().c_str());
    }
    
    // FREE
    for(auto& propagator: propagators){
        if(propagator) delete propagator;
    }
    if (ctl) { clingo_control_free(ctl); }


    AmoSumInitializer::cleanup();
    PropagatorClingoInitializer::cleanup();
    // printf("returning exit code: %d", resultOpt->exitCode);

    

    return resultOpt->exitCode;
}

PropagatorClingo* register_propagator(clingo_control_t *ctl, clingo_propagator_t prop, 
    std::string prop_type, const ParameterMap& param,
    std::vector<PropagatorClingo*> &propagators){

    std::tuple<bool, const std::vector<clingo_literal_t>* (*)(const Group*, AmoSumPropagator*), std::string> result = get_propagator_variables(prop_type);

    bool ge = std::get<0>(result);
    const std::vector<clingo_literal_t>* (*propagation_phase)(const Group*, AmoSumPropagator*) = std::get<1>(result);
    std::string choice_cons = std::get<2>(result);
    

    PropagatorClingo* propagator_clingo = new PropagatorClingo(param, propagation_phase, ge, choice_cons);

    handle_error(clingo_control_register_propagator(ctl, &prop, propagator_clingo, false));
    propagators.push_back(propagator_clingo);
    return propagator_clingo;
}
