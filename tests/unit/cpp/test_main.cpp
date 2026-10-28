// entry point of the unit tests: sets up what amoclingo-core's main sets up (ArgParse with amosum's options,
// Statistics, the logger in debug builds), then runs the registered tests
//
//   unit_tests <output dir> [filter]     filter: only the tests whose name contains it
#include <string>
#include <vector>
#include "unit.h"
#include "amosum/amoclingo/cpp/options.h"
#include "fiore/utils/argparse.h"
#include "fiore/utils/constants.h"
#include "fiore/utils/logger.h"
#include "fiore/utils/stats.h"
#include "fiore/utils/utility.h"

int main(int argc, char** argv){
    const std::string outDir = argc > 1 ? argv[1] : ".";
    const std::string filter = argc > 2 ? argv[2] : "";

    // amoclingo-core's options with their defaults; the stats and log files go to the output dir
    std::string statsArg = "--" + fiore::CONSTANTS::STATSFILEKey + "=" + outDir + "/unit_tests.stats";
    std::string logArg = "--" + fiore::CONSTANTS::LogFileNameKey + "=" + outDir + "/unit_tests.log";
    std::vector<char*> args = {argv[0], statsArg.data(), logArg.data()};

    fiore::startWallTime();
    fiore::ArgParse::init((int)args.size(), args.data(), amosumArguments());
    #ifdef __debug__
        fiore::Logger::init(fiore::ArgParse::getInstance()->get(fiore::CONSTANTS::LogFileNameKey), false,
                            fiore::Logger::stringToLevel(fiore::ArgParse::getInstance()->get(fiore::CONSTANTS::logKey)));
    #endif
    fiore::Statistics::initInstance(fiore::ArgParse::getInstance()->get(fiore::CONSTANTS::STATSFILEKey));

    const int failed = unit::runAll(filter);
    fiore::cleanup();
    return failed == 0 ? 0 : 1;
}
