// // C++ program to implement a basic logging system.



// #include <vector>
// #include <string>
// #include <ctime>
// #include <fstream>
// #include <iostream>
// #include <sstream>
// #include <assert.h>
// #include "logger.h"
// #include "utility.h"
// using namespace std;

// #ifndef NDEBUG

// Logger::Logger(const std::string& baseFilename, bool lazy, std::bitset<NUM_LOG_LEVELS> config)
//         : firstLog(true), lazy(lazy), config(config)
//     {
//         // Get current timestamp
//         time_t now = time(0);
//         tm* timeinfo = localtime(&now);
//         char timestamp[20];
//         strftime(timestamp, sizeof(timestamp), "%Y-%m-%d", timeinfo);

//         // Construct the full filename with date
//         std::string filename = baseFilename + "_" + std::string(timestamp) + ".log";
//         // std::string filename = baseFilename;

//         // Open the log file in append mode
//         #ifdef APPEND_MODE_LOG
//         logFile.open(filename, std::ios::app);
//         #else 
//         logFile.open(filename, std::ios::out | std::ios::trunc);
//         #endif

//         if (!logFile.is_open()) {
//             if(writeToFIle){
//                 writeToFIle = false ; 
//                 log(SIGINTLOGGER, "log file ", filename, " does not exist");
//             }
//         }
//         setLoggerConfig();
//         logf(INFO, "----------------------------------------------------- Logger started 🎬 ------------------------------------------------------");
// }

// void Logger::init(const std::string& baseFilename, bool lazy, std::bitset<NUM_LOG_LEVELS> config){
//     // printf("config.test(DEBUG): %d", config.test(DEBUG));
//     if(instance == nullptr){
//         instance = new Logger(baseFilename, lazy, config);
//     }
// }

// // void Logger::add_checker_stats(float time){
// //     ++stats.numberOfChecks;
// //     stats.totalTimeChecker += time;
// //     if (time > stats.maxTimeChecker) stats.maxTimeChecker = time;
// //     if (time < stats.minTimeChecker || stats.minTimeChecker == -1) stats.minTimeChecker = time ;
// // }

// // void Logger::add_reason_stats(float time, size_t length){
// //     ++stats.numberOfReason;

// //     stats.totalLengthReasons += length;
// //     if (length >= stats.maxLengthReason) stats.maxLengthReason = length  ;
// //     if (length <= stats.minLengthReason || stats.minLengthReason == -1) stats.minLengthReason = length  ;

// //     stats.totalTimeReason += time;
// //     if (time > stats.maxTimeReason) stats.maxTimeReason = time ;
// //     if (time < stats.minTimeReason || stats.minTimeReason == -1) stats.minTimeReason = time ;

// // }

// Logger* Logger::getInstance(){
//     assert (instance != nullptr);
//     return instance;
// }

// void Logger::cleanup(){
//     if(instance) delete instance;
// }

// // Destructor: Closes the log file
// Logger::~Logger() { 
//     if(lazy){
//         for(auto& message : messages){
//             if (writeToFIle && logFile.is_open()) {
//                 logFile << message;
//             }
//         }
//         logFile.flush(); 
//     }
//     logf(INFO, "----------------------------------------------------- Logger finished 🏁 ------------------------------------------------------");
//     logFile.close(); 
// }

// // Logs a message with a given log level
// void Logger::__log__(LogLevel level, const string& message)
// {
//     // Get current timestamp
//     time_t now = time(0);
//     tm* timeinfo = localtime(&now);
//     char timestamp[20];
//     strftime(timestamp, sizeof(timestamp),
//              "%Y-%m-%d %H:%M:%S", timeinfo);

//     // Create log entry
//     ostringstream logEntry;
//     logEntry << "[" << std::string(timestamp) << "] "
//              << levelToString(level) << ": " << message
//              << endl;

//     const string& logEntryStr = (firstLog ? "\n" : "") + logEntry.str();
//     firstLog = false;

//     if(lazy){
//         messages.push_back(logEntryStr);
//     }else{
//         if(print_b) std::cerr << logEntryStr;
//         if (writeToFIle && logFile.is_open()) {
//             logFile << logEntryStr;
//             logFile.flush(); 
//         }
//     }
// }

// void Logger::setLoggerConfig(){
//     #ifdef NOINFO
//         config.reset(INFO);
//         print("INFO DEACTIVATED");
//     #endif
//     #ifdef NODEBUG
//         config.reset(DEBUG);
//         print("DEBUG DEACTIVATED");
//     #endif
//     #ifndef STATSCLINGO
//         config.reset(STATS);
//         print("STATSCLINGO DEACTIVATED");
//     #endif
// }

// inline Logger* Logger::instance = nullptr;


// #endif