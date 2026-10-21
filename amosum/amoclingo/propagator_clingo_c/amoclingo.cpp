// amosum_lsu: the amosum solver on fiore (AmosumClingoSolver + AmoclingoPropagator), with a linear SAT-UNSAT search
// for the optimization problems
//
//   amosum_lsu [--reason=nomin|ijcai|min|minfly|cmin] [--lazy=false|true|hybrid|p] [fiore options] <rewritten ASP files>
//
// The ASP files are the output of the amosum parser (amosumparser). An #amomaximize is maximized by asking, after each
// model, for a value one higher; otherwise a #minimize is minimized by asking for a cost one lower (fiore's
// PB propagator); otherwise the first answer set is the result. Exit codes: SETTINGS::SAT, UNSAT, OPTIMUM, UNKNOWN
// (interrupted: the best answer set found so far is printed).

#include <csignal>
#include <cstdio>
#include <algorithm>
#include <string>
#include <vector>
#include <unistd.h>
#include "../../amosum_solver.h"
#include "../../argparse.h"
#include "../../settings.h"
#include "fiore/utils/logger.h"
#include "fiore/utils/stats.h"
#include "fiore/model/assigment.h"

namespace {

    // the best answer set printed so far: printed again (as not proved optimal) if the search is interrupted
    std::string bestAnswer;

    void onSignal(int){
        const std::string out = bestAnswer.empty() ? "UNKNOWN\n" : bestAnswer + "UNKNOWN\n";
        // write is async-signal-safe, printf is not
        ssize_t written = write(STDOUT_FILENO, out.data(), out.size());
        (void)written;
        _exit(SETTINGS::UNKNOWN);
    }

    // the true atoms of the answer set, without the atoms introduced by the rewriting (__group__, __element__, ...)
    std::string atomsOf(const fiore::Program* program, const fiore::Assignment* answerset){
        std::vector<std::string> atoms;
        for(const fiore::Vector<const fiore::Literal*>* literals: {program->restLiterals.get(), program->softLiterals.get()}){
            for(const fiore::Literal* l: *literals){
                if(l->id <= 0 || l->name.rfind("__", 0) == 0) continue;
                if(answerset->isTrue(l)) atoms.push_back(l->name);
            }
        }
        std::sort(atoms.begin(), atoms.end());
        std::string out;
        for(size_t i = 0; i < atoms.size(); ++i) out += (i == 0 ? "" : " ") + atoms[i];
        return out;
    }

    std::string answerText(const size_t n, const std::string& atoms, const std::string& objective){
        std::string out = "Answer: " + std::to_string(n) + "\n" + atoms + "\n";
        if(!objective.empty()) out += objective + "\n";
        return out;
    }
}

int main(int argc, char** argv){
    fiore::startWallTime();
    try{
        fiore::ArgParse::init(argc, argv, amosumArguments());
    }catch(const Args::HelpHasBeenPrintedException&){
        return 0;
    }catch(const Args::BaseException& e){
        // a value not accepted, an unknown option, ...
        std::fprintf(stderr, "amosum_lsu: %s\n", e.desc().c_str());
        return fiore::CONSTANTS::ERROR_CODE;
    }
    fiore::ArgParse* args = fiore::ArgParse::getInstance();
    #ifdef __debug__
        fiore::Logger::init(args->get(fiore::CONSTANTS::LogFileNameKey), false, fiore::Logger::stringToLevel(args->get(fiore::CONSTANTS::logKey)));
    #endif
    fiore::Statistics::initInstance(args->get(fiore::CONSTANTS::STATSFILEKey));
    std::signal(SIGINT, onSignal);
    std::signal(SIGTERM, onSignal);

    const std::vector<std::string> inputs(args->positional().begin(), args->positional().end());
    if(inputs.empty()) fiore::setExitCode(fiore::CONSTANTS::ERROR_CODE, true, "no ASP file given (the output of amosumparser)");

    int exitCode;
    {   // the solver is destroyed before fiore::cleanup (the logger may be used while destroying it)
    AmosumClingoSolver solver(args->get(AMOSUM_ARGS::reasonKey), args->get(AMOSUM_ARGS::lazyKey));
    solver.init(inputs);

    // LSU: the whole search in every call, no assumptions, no cores
    solver.setNoConflictLimits();
    solver.clearAssumptions();
    solver.disableGetDerivedFromAssDetection();
    solver.disableRestDetection();
    solver.setCollectUnsatCores(false);

    AmoclingoPropagator* maximizer = solver.maximizer;
    const bool minimize = maximizer == nullptr && solver.program->isOptimizationProblem;
    if(maximizer != nullptr && solver.program->isOptimizationProblem)
        fiore::setExitCode(fiore::CONSTANTS::ERROR_CODE, true, "#amomaximize together with #minimize is not supported");
    fiore::PBConstraint* optimizer = minimize ? solver.createOptimizer(fiore::CONSTANTS::INFPlus64) : nullptr;
    const bool optimization = maximizer != nullptr || minimize;

    size_t models = 0;
    while(true){
        std::pair<fiore::ClingoAssignment*, const fiore::clingo::ClingoResult> result = solver.solve();
        if(!result.second.isSat()) break;
        ++models;
        std::string objective;
        if(maximizer != nullptr){
            // the value of the model, from the last total assignment (checkAmosum); then ask for a higher one
            const int64_t value = maximizer->lastTotalValue;
            objective = "Value: " + std::to_string(value);
            maximizer->pbConstraint->updateBound(value + 1);
        }else if(minimize){
            const int64_t cost = result.first->cost();
            objective = "Optimization: " + std::to_string(cost);
            optimizer->updateBound(cost - 1);
        }
        bestAnswer = answerText(models, atomsOf(solver.program.get(), result.first), objective);
        std::printf("%s", bestAnswer.c_str());
        std::fflush(stdout);
        fiore::AnswersetsPool::getInstance()->deleteAnswerset(result.first);
        if(!optimization) break;
    }

    if(models == 0){
        std::printf("UNSATISFIABLE\n");
        exitCode = SETTINGS::UNSAT;
    }else if(optimization){
        std::printf("OPTIMUM FOUND\n");
        exitCode = SETTINGS::OPTIMUM;
    }else{
        std::printf("SATISFIABLE\n");
        exitCode = SETTINGS::SAT;
    }
    std::printf("Models: %zu\n", models);
    }

    fiore::cleanup();
    return exitCode;
}
