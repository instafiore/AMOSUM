#include "amosum/amoclingo/cpp/runner.h"
#include "amosum/amoclingo/cpp/solver.h"
#include "amosum/amoclingo/cpp/options.h"
#include "amosum/core/cpp/search/algorithms.h"

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

    std::unique_ptr<Progression> progression;
    try{
        progression = std::make_unique<Progression>(args->get(AMOSUM_ARGS::progressionKey));
    }catch(const std::invalid_argument& e){
        fiore::setExitCode(fiore::CONSTANTS::ERROR_CODE, true, e.what());
    }

    AmosumClingoBackend backend(solver);
    fiore::clingo::ClingoResult result;
    if(algorithm == AMOSUM_ARGS::algorithmSatUnsat || !hasAmomaximize) result = satUnsat(backend, *progression);
    else if(algorithm == AMOSUM_ARGS::algorithmUnsatSat) result = unsatSat(backend, *progression);
    else if(algorithm == AMOSUM_ARGS::algorithmBinary) result = binarySearch(backend);     // --progression not used
    else fiore::setExitCode(fiore::CONSTANTS::ERROR_CODE, true, "Unknown algorithm: " + algorithm);

    std::ostringstream out;
    std::string answerOutString;
    fiore::AnswersetsPool* pool = fiore::AnswersetsPool::getInstance();
    if(result.isSat()){
        fiore::Assignment* answerSet = pool->getBestKnownAnswerSet();
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