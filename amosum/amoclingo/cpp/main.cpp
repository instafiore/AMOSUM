#include <clingo.h>
#include <stdlib.h>
#include <stdio.h>
#include <string>
#include <assert.h>
#include "amosum/core/cpp/utility.h"
#include <sstream>
#include <iostream>
#include <vector>
#include <limits>
#include <chrono>
#include <csignal>
#include <ctime>
#include <unistd.h>
#include "fiore/utils/constants.h"
#include "fiore/utils/logger.h"
#include "fiore/utils/utility.h"
#include "amosum/amoclingo/cpp/propagator.h"
#include "fiore/model/propagatorPBConstraint.h"
#include "fiore/model/assigment.h"
#include "fiore/utils/utilityCommon.h"
#include "fiore/utils/argparse.h"
#include "amosum/amoclingo/cpp/options.h"
#include "fiore/utils/stats.h"
#include "fiore/clingo/utilityClingo.h"
#include "amosum/amoclingo/cpp/runner.h"
#include "fiore/clingo/stop.h"



using namespace Args;
using fiore::operator<<;


// SIGINT / SIGTERM: only async-signal-safe operations (see fiore/clingo/stop.h). The run stops at the solve call in
// progress and prints its best answer set in normal code (Runner::run), then main writes the statistics and exits.
void signalHandler(int signum) {
    static timespec first = {0, 0};
    timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);
    if(fiore::clingo::stopRequested){
        // `timeout` signals the process and its process group, and the amoclingo wrapper forwards it too: the same
        // signal arrives two or three times at once -> ignored. One >= 2 s after the first means the run did not
        // stop -> quit now, with the "killed by signal" code
        const long long elapsedNs = (now.tv_sec - first.tv_sec) * 1000000000LL + (now.tv_nsec - first.tv_nsec);
        if(elapsedNs >= 2000000000LL) _exit(128 + signum);
        return;
    }
    first = now;
    fiore::clingo::stopRequested = 1;
    clingo_control_t* control = fiore::clingo::controlToInterrupt.load();
    if(control != nullptr) clingo_control_interrupt(control);      // documented as callable from a signal handler
}

static void installSignalHandler() noexcept{
    struct sigaction action{};
    action.sa_handler = signalHandler;
    sigemptyset(&action.sa_mask);
    action.sa_flags = SA_RESTART;
    sigaction(SIGINT, &action, nullptr);
    sigaction(SIGTERM, &action, nullptr);
}

int main(int argc, char ** argv) {

    try {

        installSignalHandler();

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
        std::cout << "amoclingo version 3.0 mode: "
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
    
        
        Runner runner;
        fiore::Statistics::initInstance(fiore::ArgParse::getInstance()->get(fiore::CONSTANTS::STATSFILEKey));
        fiore::clingo::ClingoResult result = runner.run();
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

