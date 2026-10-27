#include "runner_clingo.h"
#include "amosum/amosum_solver.h"
#include "amosum/amosum_argparse.h"

using namespace Args;
fiore::clingo::ClingoResult Runner::run(){

    auto start = fiore::startTimer();

    fiore::ArgParse* args = fiore::ArgParse::getInstance();
    StringList ASPInputs = args->cmd.positional();
    if(args->isDefined("ASPInputs")){
        std::string ASPInputsParametersString = args->get("ASPInputs");
        StringList ASPInputsParameters = split(ASPInputsParametersString,'&');
        fiore::extendVector(ASPInputs, ASPInputsParameters);
    }

    #ifdef __debug__
        LOG(fiore::LogLevel::INFO_1," Starting amoclingo");
    #endif
    
    fiore::Assignment* answerSet = nullptr;
    AmosumClingoSolver * solver = new AmosumClingoSolver(args->get(AMOSUM_ARGS::reasonKey), args->get(AMOSUM_ARGS::lazyKey));
    solver->init(ASPInputs);

    std::ostringstream out;
    std::string answerOutString = answerSet->toOutputString();
    out<<answerOutString;
    const double elasped = fiore::elapsedTime(start);
    out<<"\nTime: "<<elasped<<"s"<<std::endl;

    std::string outString = out.str();
    std::cout<<outString<<std::endl;

    #ifdef __debug__
        LOG(fiore::LogLevel::ALWAYS,answerOutString);
    #endif

    if(solver != nullptr) delete solver;
    const fiore::clingo::ClingoResult result = answerSet->result;
    fiore::AnswersetsPool::getInstance()->deleteAnswerset(answerSet);
    return result;
}