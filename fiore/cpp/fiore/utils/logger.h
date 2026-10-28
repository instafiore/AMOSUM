// C++ program to implement a basic logging system.
#pragma once
#include <vector>
#include <string>
#include <ctime>
#include <fstream>
#include <iostream>
#include <sstream>
#include <assert.h>
#include "utilityCommon.h"
#include "constants.h"
#include "args-parser/args-parser/all.hpp"
#include <stdexcept>



namespace fiore {


#ifndef NDEBUG
    #define __debug__
#endif


// Enum to represent log levels
enum LogLevel {DEBUG=10, DEBUG_1=11, DEBUG_2=12, DEBUG_3=13, INFO=20, INFO_1=21, INFO_2=22, CLINGO_WARNING = 25, WARNING=30, CLINGO_ERROR = 35, CRITICAL=40, ERROR=50, UNKNOWN = 100, ALWAYS = 1000};

const int NUM_LOG_LEVELS = 11;

std::unordered_set<std::string> getLogLevelNames();

class Logger {
private:
    static Logger* instance;
   
    bool firstLog = true;
    bool lazy = false;
    std::vector<std::string> messages;
    bool writeToFIle = true ;       
    
    
    // Constructor: Opens the log file in append mode
    Logger(const std::string& baseFilename, bool lazy, LogLevel level);

    std::ofstream logFile; // File stream for the log file

    // Converts log level to a std::string for output


    template<typename... Args>
    void printLog(LogLevel level, const Args&... message){
        if (this->level <= level) {
            std::ostringstream oss;
            (oss << ... << message); 
            std::string message_str = oss.str();
            __log__(level, message_str);
            if(level == ERROR){
                setExitCode(CONSTANTS::ERROR_CODE);
                exit(CONSTANTS::ERROR_CODE);
            }
        }
    }

    // Logs a message with a given log level
    void __log__(LogLevel level, const std::string& message);

public:
    // StatsStruct stats ;
    LogLevel level;
    static void init(const std::string& baseFilename, bool lazy = false, LogLevel level = LogLevel::DEBUG);
    static Logger* getInstance();
    static void cleanup();
    ~Logger();

    template<typename... Args>
    void log(const Args&... message)
    {
        printLog(DEBUG, message...);
    }

   
    template<typename... Args>
    void log(LogLevel level, const Args&... message)
    {   
        printLog(level, message...);
    }

    static std::string levelToString(LogLevel level)
    {
        switch (level) {
        case INFO:
            return "INFO";
        case INFO_1:
            return "INFO-1";
        case INFO_2:
            return "INFO-2";
        case DEBUG:
            return "DEBUG";
        case DEBUG_1:
            return "DEBUG-1";
        case DEBUG_2:
            return "DEBUG-2";
        case DEBUG_3:
            return "DEBUG-3";
        case ERROR:
            return "ERROR";
        case WARNING:
            return "WARNING";
        case CLINGO_WARNING:
            return "CLINGO-WARNING";
        case CRITICAL:
            return "CRITICAL";
        case ALWAYS:
            return "ALWAYS";
        default:
            return "UNKNOWN";
        }
    }

    static LogLevel stringToLevel(std::string level)
    {
        if(level == "INFO"){
            return LogLevel::INFO;
        }else if (level == "INFO_1")
        {
            return LogLevel::INFO_1;
        }
        else if (level == "INFO_2")
        {
            return LogLevel::INFO_2;
        }
        else if (level == "DEBUG")
        {
            return LogLevel::DEBUG;
        }else if (level == "DEBUG_1")
        {
            return LogLevel::DEBUG_1;
        }else if (level == "DEBUG_2")
        {
            return LogLevel::DEBUG_2;
        }else if (level == "DEBUG_3")
        {
            return LogLevel::DEBUG_3;
        }else if (level == "ERROR")
        {
            return LogLevel::ERROR;
        }else if (level == "CRITICAL")
        {
            return LogLevel::CRITICAL;
        }else if (level == "ALWAYS")
        {
            return LogLevel::ALWAYS;
        }else
        {
            Args::BaseException exception("Log level "+level+" not valid");
            throw exception;
        }
        return LogLevel::ERROR;
    }
    
};


// #define log(logLevel, ...) Logger::getInstance()->log(logLevel, __VA_ARGS__, " @ ", __FILE__,":",__LINE__)
#ifdef __debug__
    #define LOG(logLevel, ...) ::fiore::Logger::getInstance()->log(logLevel, __VA_ARGS__, " @ ", __FILE__,":",__LINE__)
#else 
    #define LOG(logLevel, ...)
#endif

} // namespace fiore
