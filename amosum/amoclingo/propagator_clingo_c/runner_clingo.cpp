#include "runner_clingo.h"
#include "amosum/amosum_solver.h"
#include "amosum/amosum_argparse.h"
#include "amoclingo_algorithms.h"

using namespace Args;
fiore::clingo::ClingoResult Runner::run(){

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

    AmosumClingoSolver solver(args->get(AMOSUM_ARGS::reasonKey), args->get(AMOSUM_ARGS::lazyKey));
    solver.init(ASPInputs);

    const bool hasAmomaximize = static_cast<AmosumProgram*>(solver.program.get())->amomaximizeConstraint != nullptr;
    const std::string algorithm = args->get(AMOSUM_ARGS::algorithmKey);

    fiore::clingo::ClingoResult result;
    if(algorithm == AMOSUM_ARGS::algorithmLsu || !hasAmomaximize) result = lsu(solver);
    else fiore::setExitCode(fiore::CONSTANTS::ERROR_CODE, true, "Unknown algorithm: " + algorithm);

    std::ostringstream out;
    std::string answerOutString;
    fiore::AnswersetsPool* pool = fiore::AnswersetsPool::getInstance();
    if(result.isSat()){
        fiore::Assignment* answerSet = pool->getOptimalAnswerSet();
        answerOutString = answerSet->toOutputString();
        pool->deleteAnswerset(answerSet);       // before the solver: it points to the solver's program
    }
    else answerOutString = result.isUnsat() ? "UNSATISFIABLE" : "UNKNOWN";

    out<<answerOutString;
    std::cout<<out.str()<<std::endl;

    #ifdef __debug__
        LOG(fiore::LogLevel::ALWAYS,answerOutString);
    #endif

    return result;
}