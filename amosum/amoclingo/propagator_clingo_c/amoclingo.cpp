#include <clingo.h>
#include <stdlib.h>
#include <stdio.h>
#include <string>
#include <assert.h>
#include "../../utility.h"
#include <sstream>
#include <iostream>
#include <vector>
#include <limits>
#include <chrono>
#include <csignal>
#include <sys/signal.h>
#include "fiore/utils/constants.h"
#include "fiore/utils/logger.h"
#include "fiore/utils/utility.h"
#include "amoclingo_propagator.h"
#include "fiore/model/propagatorPBConstraint.h"
#include "fiore/model/assigment.h"
#include "fiore/utils/utilityCommon.h"
#include "fiore/utils/argparse.h"
#include "amosum/amosum_argparse.h"
#include "fiore/utils/stats.h"
#include "fiore/clingo/utilityClingo.h"




using namespace Args;
using fiore::operator<<;


void signalHandler(int signum) {
    fiore::Assignment* optAnswer = fiore::AnswersetsPool::getInstance()->getOptimalAnswerSet();
    optAnswer->result.unkown();
    optAnswer->printAssignment();
    fiore::setExitCode(optAnswer->result.exitCode());
    fiore::cleanup();
    exit(fiore::CONSTANTS::EXIT_CODE);
}

int main(int argc, char ** argv) {

    try {

        signal(SIGINT, signalHandler);
        signal(SIGTERM, signalHandler);

        #ifdef NDEBUG
            static constexpr bool test_mode    = false;
        #else
            static constexpr bool test_mode    = true;
        #endif

        #ifdef NSTATS
            static constexpr bool stats_mode = false;
        #else
            static constexpr bool stats_mode = true;
        #endif

        static constexpr bool release_mode = !test_mode;
        std::cout << "weaksbls version 2.0 mode: "
                << (release_mode ? "release" : "test")
                << (stats_mode   ? "-stats"  : "")
                << "\n";

        fiore::startWallTime();

        fiore::ArgParse::init(argc, argv, amosumArguments());
        fiore::ArgParse* argparse = fiore::ArgParse::getInstance();
        
        StringList ASPInputsPositional = argparse->positional();
        if(argparse->isDefined("ASPInputs")){
            std::string ASPInputsParametersString = argparse->get("ASPInputs");
            StringList ASPInputsParameters = split(ASPInputsParametersString,'&');
            fiore::extendVector(ASPInputsPositional, ASPInputsParameters);
        }    

        if(ASPInputsPositional.size() == 0){
            throw BaseException("At least one ASP Input is required!");
        }

        #ifdef  __debug__
            std::string pathToLogFile = argparse->get(fiore::CONSTANTS::LogFileNameKey);
            bool lazyLogger = false;
            fiore::LogLevel logLevel = fiore::Logger::stringToLevel(argparse->get(fiore::CONSTANTS::logKey));
            fiore::Logger::init(pathToLogFile, lazyLogger, logLevel);
            LOG(fiore::LogLevel::INFO, std::string("ASP input files: "),ASPInputsPositional);
            std::cout<<"Debugging at: "<<pathToLogFile<<"\n";
        #endif
    
        std::ostringstream oss;
        oss<<std::string("ASP input files: ")<<ASPInputsPositional;
        std::cout<<oss.str()<<std::endl;
        
        // Runner runner;
        fiore::Statistics::initInstance(fiore::ArgParse::getInstance()->get(fiore::CONSTANTS::STATSFILEKey));
        // fiore::clingo::ClingoResult result = runner.run();
        fiore::clingo::ClingoResult result;
        fiore::setExitCode(result.exitCode());
    } catch( Args::HelpHasBeenPrintedException & e) {
        return 0;
    } catch( const BaseException & x ) {
        outStream() << x.desc() << std::endl;
        fiore::setExitCode(fiore::CONSTANTS::ERROR_CODE, true);
    }catch (const std::exception & e) {
        // Standard library exceptions (std::runtime_error, std::bad_alloc, etc.)
        outStream() << "Unhandled std::exception: " << e.what() << std::endl;
        fiore::setExitCode(fiore::CONSTANTS::ERROR_CODE, true);

    } catch (...) {
        // Catch everything else (non-std exceptions, unknown)
        outStream() << "Unknown error occurred!" << std::endl;
        fiore::setExitCode(fiore::CONSTANTS::ERROR_CODE, true);
    }

    fiore::cleanup();
    return fiore::CONSTANTS::EXIT_CODE;
}

