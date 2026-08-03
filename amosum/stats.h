// #pragma once
// #include <vector>
// #include <string>
// #include <ctime>
// #include <fstream>
// #include <iostream>
// #include <sstream>
// #include <assert.h>
// #include "utility.h"


// #ifndef NSTATS

// #define startTimer()\
// start_timer();
// #define displayEndTimer(start, name)\
// display_end_timer(start, name);
// #define prinStats(force)\
// print_stats_oneline(force);
// #define addValue(name, value)\
// HandleStats::getInstance()->add_value(name, value);
// #define addCheckerStats(t)\
// Logger::getInstance()->add_checker_stats(t);
// #define addReasonStats(t, length)\
// Logger::getInstance()->add_reason_stats(t, length);
// #define increaseCountStats()\
// HandleStats::getInstance()->incrementCount();
// #define resetCountStats()\
// HandleStats::getInstance()->resetCount();

// struct Stat{
     
//     std::string name;
//     size_t count = 0 ;
//     double sum = 0 ;
//     double min = 0 ;
//     double max = 0 ;
//     double mean = 0 ;
//     bool decimal = true;
//     Stat(std::string name, bool decimal=true): name(name), decimal(decimal) {}
    
//     virtual std::string toString(int ind = 0);
//     virtual std::string toStringOneline();
//     virtual void add_value(float value);

//     virtual ~Stat() = default;

// };

// struct IntStat{
//     bool decimal = false;
// };



// struct HandleStats{

//     static HandleStats* instance;
//     size_t count =  0 ;
//     static HandleStats* getInstance(){
//         assert(instance != nullptr);
//         return instance;
//     }

//     static void initInstance(std::string logFileName, size_t freshRate = 1000){
//         if(instance == nullptr){
//             instance = new HandleStats(logFileName, freshRate);
//         }
//     }

//     size_t freshRate; 
//     std::unordered_map<std::string, Stat*> stats;

    
//     virtual ~HandleStats(){
//         for(auto& [name, stat]: stats){
//             delete stat;
//         }
//     }

//     static void cleanup(){
//         if(instance != nullptr) delete instance ;
//     }

//     virtual std::string toString();
//     virtual std::string toStringOneline();
//     virtual void printStringOneline();

//     void add_stat(Stat* stat);
//     void remove_stat(Stat* stat);
//     void add_value(std::string name, float value);
//     bool canPrint(){
//         return count % freshRate == 0 && count != 0;
//     }

//     void incrementCount(){
//         count++;
//     }
//     void resetCount(){
//         count = 0 ;
//     }



// private:
//     std::ofstream logFile;
//     HandleStats(const std::string& logFileName, size_t freshRate = 1000): freshRate(freshRate){
//         logFile.open(logFileName, std::ios::out | std::ios::trunc);
//         if (!logFile.is_open()) {
//             printf("log file %s does not exist", logFileName.c_str());
//             setExitCode(SETTINGS::ERROR_CODE);
//         }
//     }
// };

// #else
//     #define startTimer()
//     #define displayEndTimer(start, name)
//     #define prinStats(force)
//     #define addCheckerStats(t)
//     #define addReasonStats(t, length)
//     #define addValue(name, value)
//     #define increaseCountStats()
//     #define resetCountStats()
// #endif