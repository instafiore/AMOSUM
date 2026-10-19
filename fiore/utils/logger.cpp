// C++ program to implement a basic logging system.



#include <vector>
#include <string>
#include <ctime>
#include <fstream>
#include <iostream>
#include <sstream>
#include <assert.h>
#include "logger.h"
#include "utility.h"


namespace fiore {

std::unordered_set<std::string> getLogLevelNames() {
    return {
        "DEBUG",
        "DEBUG_1",
        "DEBUG_2",
        "DEBUG_3",
        "INFO",
        "INFO_1",
        "INFO_2",
        "CLINGO_WARNING",
        "WARNING",
        "CLINGO_ERROR",
        "CRITICAL",
        "ERROR",
        "UNKNOWN",
        "ALWAYS"
    };
}

Logger::Logger(const std::string& baseFilename, bool lazy, LogLevel level)
        : firstLog(true), lazy(lazy), level(level)
    {
        // Get current timestamp
    
        time_t now = time(0);
        tm* timeinfo = localtime(&now);
        char timestamp[20];
        strftime(timestamp, sizeof(timestamp), "%Y-%m-%d", timeinfo);

        // Construct the full filename with date
        std::string filename = baseFilename;
        
        // the timestamp option exists only when the application uses ArgParse
        if(ArgParse::isInitialized() && ArgParse::getInstance()->cmd.isDefined("--"+CONSTANTS::TimestampToLogFileKey)) filename += std::string("_") + std::string(timestamp) + std::string(".log");

        // Open the log file in append mode
        #ifdef APPEND_MODE_LOG
        logFile.open(filename, std::ios::app);
        #else 
        logFile.open(filename, std::ios::out | std::ios::trunc);
        #endif

        if (!logFile.is_open()) {
            if(writeToFIle){
                writeToFIle = false ; 
                std::cout<<std::string("log file ")<<filename<<std::string(" does not exist")<<std::endl;
                setExitCode(ERROR, true);
            }
        }
        printLog(INFO, std::string("----------------------------------------------------- Logger started 🎬 ------------------------------------------------------"));
}

void Logger::init(const std::string& baseFilename, bool lazy, LogLevel level){
    // printf("config.test(DEBUG): %d", config.test(DEBUG));
    if(instance == nullptr){
        instance = new Logger(baseFilename, lazy, level);
    }
}

Logger* Logger::getInstance(){
    assert (instance != nullptr);
    return instance;
}

void Logger::cleanup(){
    if(instance) delete instance;
}

// Destructor: Closes the log file
Logger::~Logger() { 
    if(lazy){
        for(auto& message : messages){
            if (writeToFIle && logFile.is_open()) {
                logFile << message;
            }
        }
        logFile.flush(); 
    }
    LOG(INFO,std::string("----------------------------------------------------- Logger finished 🏁 ------------------------------------------------------"));
    logFile.close(); 
}

// Logs a message with a given log level
void Logger::__log__(LogLevel level, const std::string& message)
{
    // Get current timestamp
    time_t now = time(0);
    tm* timeinfo = localtime(&now);
    char timestamp[20];
    strftime(timestamp, sizeof(timestamp),
             "%Y-%m-%d %H:%M:%S", timeinfo);

    const double& elasped = getWallTime();
    std::string elaspedStr = std::string(" ") + std::to_string(elasped) + std::string("s ");
    // Create log entry
    std::ostringstream logEntry;
    logEntry << '[' << std::string(timestamp)<< elaspedStr << std::string("] ")
             << Logger::levelToString(level) << std::string(": ") << message
             << std::endl;

    const std::string& logEntryStr = (firstLog ? std::string("\n") : std::string("")) + logEntry.str();
    firstLog = false;

    if(lazy){
        messages.push_back(logEntryStr);
    }else{
        if (writeToFIle && logFile.is_open()) {
            logFile << logEntryStr;
            logFile.flush(); 
        }
    }
}


inline Logger* Logger::instance = nullptr;

} // namespace fiore
