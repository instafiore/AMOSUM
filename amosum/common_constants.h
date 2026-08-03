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


namespace CONSTANTS {
const int32_t NONE32 = std::numeric_limits<int32_t>::max();
const int32_t INFPlus32 = std::numeric_limits<int32_t>::max();
const int32_t INFMinus32 = std::numeric_limits<int32_t>::min();
const int64_t NONE64 = std::numeric_limits<int64_t>::max();
const int64_t INFPlus64 = std::numeric_limits<int64_t>::max();
const int64_t INFMinus64 = std::numeric_limits<int64_t>::min();
const std::string INFSTR = "inf";
const int SAT_CODE = 10;
const int UNSAT_CODE = 20;
const int OPTIMUM_CODE = 30;
const int OPTIMUM_UNKOWN_CODE = 29;
const int UNKOWN_CODE = 28;
const int ERROR_CODE = 1;
const int NOERROR_CODE = 0;
inline int EXIT_CODE = NOERROR_CODE;
const std::string TRUE_STR = "true" ;
const std::string FALSE_STR = "false" ;
const char NOT = '~';
const std::string NONE_STR = "None";
const std::regex regexNegativeLiteral(R"(not\s(.+))");
inline std::filesystem::path ROOT;
const std::string TimestampToLogFileKey = "TimestampLogFile";
const std::string LogFileNameKey = "log_file_name";
const std::string PBConstraintImplementationKey = "pb_imp";
const char BOTTOM = -1;
const char TOP = -BOTTOM;
}