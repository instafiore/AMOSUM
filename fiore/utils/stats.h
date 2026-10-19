#pragma once
#include "utility.h"
#include "logger.h"
#include <variant>
#include <type_traits>
#include <cassert>
#include <limits>
#include <random>

// a generic statistic: an integer counter, a floating point value, or a text value

namespace fiore {

using StatValue = std::variant<int64_t, double, std::string>;

// distribution of the values passed to increaseGenericStat for one key
struct StatSummary{
    static constexpr size_t SAMPLE_CAPACITY = 1024;   // exact median up to this many increases
    size_t count = 0;
    double sum = 0.0;
    double min = std::numeric_limits<double>::infinity();
    double max = -std::numeric_limits<double>::infinity();
    std::vector<double> sample;                         // reservoir sample, for the median

    void add(const double v, std::mt19937_64& rng) noexcept{
        ++count;
        sum += v;
        min = std::min(min, v);
        max = std::max(max, v);
        if(sample.size() < SAMPLE_CAPACITY) sample.push_back(v);
        else{
            std::uniform_int_distribution<size_t> pick(0, count - 1);
            const size_t j = pick(rng);
            if(j < SAMPLE_CAPACITY) sample[j] = v;
        }
    }
    double mean() const noexcept{ return count == 0 ? 0.0 : sum / (double)count; }
    double median() const noexcept;
    bool isMedianExact() const noexcept{ return count <= SAMPLE_CAPACITY; }
};

struct Statistics{

    // set: numbers keep their kind (integer or floating point), text is stored as a string
    template<typename T, std::enable_if_t<std::is_arithmetic_v<T>, int> = 0>
    void updateGenericStat(const std::string& key, const T value) noexcept { genericStats[key] = toStatValue(value); }
    void updateGenericStat(const std::string& key, const std::string& value) noexcept { genericStats[key] = value; }
    void updateGenericStat(const std::string& key, const char* value) noexcept { genericStats[key] = std::string(value); }

    // add: numbers only; an integer stat stays an integer as long as integers are added.
    // every call is also one observation of `value` in the stat's summary (count, sum, min, max, mean, median)
    template<typename T, std::enable_if_t<std::is_arithmetic_v<T>, int> = 0>
    void increaseGenericStat(const std::string& key, const T value) noexcept{
        genericSummaries[key].add((double)value, summaryRng);
        auto it = genericStats.find(key);
        if(it == genericStats.end()){ genericStats[key] = toStatValue(value); return; }
        StatValue& current = it->second;
        if(int64_t* i = std::get_if<int64_t>(&current)){
            if constexpr (std::is_integral_v<T>) *i += (int64_t)value;
            else current = (double)*i + (double)value;
        }else if(double* d = std::get_if<double>(&current)){
            *d += (double)value;
        }else{
            assert(false && "increaseGenericStat on a text statistic");
        }
    }

    std::string toString() const noexcept;
    std::string toStringStatsSolver() const noexcept;
    std::string toStringGenericStats() const noexcept;
    std::string toJson() const noexcept;

    static Statistics* getInstance() noexcept;     
    void printOnFile();   
    void printOnSTDOUT();   
    static void initInstance(std::string logFileName) noexcept;
    // std::unordered_map<std::string, double> statsClingo;
    std::unordered_map<std::string, double> statsSolver;
    std::unordered_map<std::string, double> statsLastSolve;
    std::unordered_map<std::string, StatValue> genericStats;
    std::unordered_map<std::string, std::vector<std::pair<double, double>>> series;
    inline void addSeriesPoint(const std::string& key, const double value) noexcept{
        std::vector<std::pair<double, double>>& s = series[key];
        if(!s.empty() && s.back().second == value) return;
        s.emplace_back(getWallTime(), value);
    }
    std::unordered_map<std::string, StatSummary> genericSummaries;   // only keys that were increased
    #ifdef __debug__
    std::unordered_map<std::string, std::unordered_map<std::string, double>> customStatsSolver;
    #endif  
    
    
private:
    static Statistics* instance;
    std::ofstream logFile;
    std::mt19937_64 summaryRng{0};   // own generator: the stats never change the search's random sequence
    Statistics(const std::string& logFileName){
        #ifndef NSTATS
            logFile.open(logFileName, std::ios::out | std::ios::trunc);
            if (!logFile.is_open()) {
                printf("log file %s does not exist", logFileName.c_str());
                setExitCode(CONSTANTS::ERROR_CODE, true);
            }
        #endif
    }

    template<typename T>
    static StatValue toStatValue(const T value) noexcept{
        if constexpr (std::is_integral_v<T>) return (int64_t)value;
        else return (double)value;
    }
};

std::ostream& operator<<(std::ostream& out, const Statistics& stats);

} // namespace fiore
