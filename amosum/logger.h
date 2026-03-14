// // C++ program to implement a basic logging system.
// #pragma once
// #include <vector>
// #include <string>
// #include <ctime>
// #include <fstream>
// #include <iostream>
// #include <sstream>
// #include <assert.h>
// #include "utility.h"

// using namespace std;

// // Enum to represent log levels
// enum LogLevel { INFO = 0, DEBUG, SIGINTLOGGER, ERROR, CRITICAL, CLINGO, STATS };
// const int NUM_LOG_LEVELS = 7;

// // #define NOINFO
// // #define NODEBUG
// // #define STATSCLINGO

// // #define APPEND_MODE_LOG


// #ifndef NDEBUG
//     #define debug(level,...) \
//     Logger::getInstance()->log(level, __VA_ARGS__);
//     #define debugf(level,...) \
//     Logger::getInstance()->logf(level, __VA_ARGS__);
//     #define initLogger(logfile, lazy_logger) \
//     Logger::init(log_file, lazy_logger);
//     #define cleanupLogger() \
//     Logger::getInstance()->cleanup();   
//     #define vectorToString(vec, name)\
//     vector_to_string(vec, name)
//     #define vectorToStringName(atomNames,vec, name)\
//     vector_to_string_name(atomNames,vec, name)
//     #define arrayToString(vec, size, name)\
//     array_to_string(vec,size,name)
//     #define unorderedSetToString(uset, name)\
//     unordered_set_to_string(uset, name)
//     #define unorderedSetToStringName(atomNames, uset, name)\
//     unordered_set_to_string_name(atomNames, uset, name)
//     #define printPropagate(prop,changes,size,control,dl,force_print)\
//     print_propagate(prop, changes, size, control, dl, force_print)
//     #define printReason(atomNames, reason, false)\
//     print_reason(atomNames, reason->data(), false)
//     #define printUndo(this, changes, size, control, dl, td, false)\
//     print_undo(this, changes, size, control, dl, td, false);
//     #define printClingoStats(stats, key, depth)\
//     print_clingo_statistics(stats, key, depth)
// class Logger {
// private:
//     static Logger* instance;
   
//     bool firstLog = true;

//     bool lazy = false;
//     std::vector<std::string> messages;

//     bool writeToFIle = true ;
//     bool print_b = false ;
    
//     // Constructor: Opens the log file in append mode
//     Logger(const std::string& baseFilename, bool lazy, std::bitset<NUM_LOG_LEVELS> config);

//     ofstream logFile; // File stream for the log file

//     // Converts log level to a string for output
//     string levelToString(LogLevel level)
//     {
//         switch (level) {
//         case DEBUG:
//             return "DEBUG";
//         case INFO:
//             return "INFO";
//         case SIGINTLOGGER:
//             return "SIGINT";
//         case ERROR:
//             return "ERROR";
//         case CRITICAL:
//             return "CRITICAL";
//         case CLINGO:
//             return "CLINGO";
//         case STATS:
//             return "STATS";
//         default:
//             return "UNKNOWN";
//         }
//     }

//     template<typename... Args>
//     void printLog(LogLevel level, bool force_print, const Args&... message){
//         if (force_print || config.test(level)) {
//             std::ostringstream oss;
//             (oss << ... << message); 
//             std::string message_str = oss.str();
//             __log__(level, message_str);
//             if(level == ERROR || level == CRITICAL){
//                 setExitCode(SETTINGS::ERROR_CODE);
//             }
//         }
//     }

// public:
//     // StatsStruct stats ;
//     std::bitset<NUM_LOG_LEVELS> config;
//     static void init(const std::string& baseFilename, bool lazy = false, std::bitset<NUM_LOG_LEVELS> config = std::bitset<NUM_LOG_LEVELS>().set());
//     static Logger* getInstance();
//     static void cleanup();
//     ~Logger();

//     void add_checker_stats(float time);
//     void add_reason_stats(float time, size_t length);

//     template<typename... Args>
//     void log(const Args&... message)
//     {
//         log(DEBUG, message...);
//     }

   
//     template<typename... Args>
//     void log(LogLevel level, const Args&... message)
//     {   
//         printLog(level, false, message...);
//     }

//     template<typename... Args>
//     void logf(LogLevel level, const Args&... message)
//     {
//         printLog(level, true, message...);
//     }

//     template<typename... Args>
//     void logf(const Args&... message)
//     {
//         printLog(DEBUG, true, message...);
//     }

//     // Logs a message with a given log level
//     void __log__(LogLevel level, const string& message);
//     void setLoggerConfig();
  
// };

// #else
//     #define debug(level,...)
//     #define initLogger(logfile, lazy_logger)
//     #define cleanupLogger()
//     #define debugf(level,...) 
//     #define vectorToString(vec, name)
//     #define vectorToStringName(vec, name)
//     #define arrayToString(vec, size, name)
//     #define unorderedSetToString(uset, name)
//     #define unorderedSetToStringName(atomNames, uset, name)
//     #define printPropagate(prop,changes,size,control,dl,force_print)
//     #define printReason(atomNames, reason, false)
//     #define printUndo(this, changes, size, control, dl, td, false)
// #endif