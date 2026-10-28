#pragma once
#include <unordered_map>
#include <iostream>
#include <vector>
#include <string>
#include <regex>
#include <stdexcept>
#include <limits>
#include <filesystem>

#define NOOP (void(0))
#define DUMMY printf("DUMMY\n")

namespace fiore {

namespace CONSTANTS {
// sentinels
const int32_t NONE32 = std::numeric_limits<int32_t>::max();
const int32_t INFPlus32 = std::numeric_limits<int32_t>::max();
const int32_t MAXTIME = INFPlus32-1;
const int32_t INFMinus32 = std::numeric_limits<int32_t>::min();
const int64_t NONE64 = std::numeric_limits<int64_t>::max();
const int64_t INFPlus64 = std::numeric_limits<int64_t>::max();
const int64_t INFMinus64 = std::numeric_limits<int64_t>::min();
const std::string INFSTR = "inf";
const std::string NONE_STR = "None";
const int32_t BOTTOM = -1;
const int32_t TOP = -BOTTOM;

// exit codes
const int SAT_CODE = 10;
const int UNSAT_CODE = 20;
const int OPTIMUM_CODE = 30;
const int OPTIMUM_UNKOWN_CODE = 29;
const int UNKOWN_CODE = 28;
const int ERROR_CODE = 1;
const int NOERROR_CODE = 0;
inline int EXIT_CODE = NOERROR_CODE;

const std::string EVENT = "event";
const std::string TRUE_STR = "t" ;
const std::string FALSE_STR = "f" ;
const char NOT = '~';
const std::regex regexNegativeLiteral(R"(not\s(.+))");
inline std::filesystem::path ROOT;

// command-line options read by the library (see ArgParse)
const std::string aspInputsKey = "asp-input";
const std::string logKey = "log";
const std::string LogFileNameKey = "log-file-name";
const std::string LOGFILE = "fiore.log";
const std::string TimestampToLogFileKey = "TimestampLogFile";
const std::string STATSFILEKey = "statsfile";
const std::string STATSFILEDefault = "fiore.stats";
const std::string seedKey = "seed";
const std::string DefaultSeed = "42";
const std::string verboseKey = "verbose";
const std::string verboseLevel1 = "1";
const std::string verboseLevel2 = "2";
const std::string verboseDef = verboseLevel1;

// polarity heuristic of ClingoPropagatorInitializer (torc: rest literals for ClingoSolver::setTORC)
const std::string polarityHeuristic = "polarity-heuristic";
const std::string polarityHeuristicPol0 = "pol0";
const std::string polarityHeuristicTORC= "torc";
const std::string defaultPolarityHeuristic = polarityHeuristicPol0;
// detection of the literals derived from each assumption (PropagatorGetDerivedFromAss)
const std::string enableAlreadyDerivedFromAssumptionDetectionKey = "dba";
const std::string enableAlreadyDerivedFromAssumptionDetectionDef = TRUE_STR;
// shuffling point of LowerBoundOBV
const std::string LOWER_BOUND_SHUFFLING_KEY = "lb-progr-shuffle-mb";
const std::string LOWER_BOUND_SHUFFLING_SELECTING_POINT_KEY = "lb-progr-shuffle-select-point-strategy-mb";
const std::string LOWER_BOUND_SHUFFLING_SELECTING_POINT_MaxIncrease = "max-increase";
const std::string LOWER_BOUND_SHUFFLING_SELECTING_POINT_Default = LOWER_BOUND_SHUFFLING_SELECTING_POINT_MaxIncrease;
}

} // namespace fiore
