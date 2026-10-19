#pragma once
#include <vector>
#include "args-parser/args-parser/all.hpp"
#include <chrono>
#include <unordered_map>
#include <unordered_set>
#include <assert.h>
#include <ostream>
#include <algorithm>
#include <sstream>
#include "constants.h"
#include <compare>
#include <any>
#include <cmath>
#include "utilityCommon.h"
#include <cstddef>


namespace fiore {

inline Clock::time_point wallTime;



// // Functions

// Classes

struct Argument{
    const std::string name;
    //! Is this argument with value?
    const bool isWithValue;
    //! Is this argument required?
    const bool isRequired;
    //! Description of the argument.
    const std::string desc;
    //! Long description.
    const std::string longDesc;
    //! Default value.
    const std::string defaultValue;
    //! Value specifier.
    const std::string valueSpecifier;

    const std::unordered_set<std::string> validValues;
    const PrintableRegex regex;

    Argument(const std::string &name, const bool &isWithValue, const bool &isRequired, const std::string &desc, const std::string &longDesc, const std::string &defaultValue, const std::string &valueSpecifier, std::unordered_set<std::string>&& validValues, const PrintableRegex& regex): 
        name(name), isWithValue(isWithValue), isRequired(isRequired), desc(desc), longDesc(longDesc), defaultValue(defaultValue), valueSpecifier(valueSpecifier), validValues(validValues), regex(regex){}

    class Builder{
    
    private:
        std::string nameBuilder;
        //! Is this argument with value?
        bool isWithValueBuilder;
        //! Is this argument required?
        bool isRequiredBuilder;
        //! Description of the argument.
        std::string descBuilder;
        //! Long description.
        std::string longDescBuilder;
        //! Default value.
        std::string defaultValueBuilder;
        //! Value specifier.
        std::string valueSpecifierBuilder;
        std::unordered_set<std::string> validValuesBuilder;
        PrintableRegex regexBuilder;
        std::unique_ptr<Argument> arg;
        bool built;

    public:
        Builder(std::string name): nameBuilder(name), isWithValueBuilder(false), isRequiredBuilder(false),
            descBuilder(std::string()), longDescBuilder(std::string()), defaultValueBuilder(std::string()), valueSpecifierBuilder(std::string()),
            validValuesBuilder(std::unordered_set<std::string>()), regexBuilder(PrintableRegex()), arg(nullptr), built(false){}

        inline Builder& isWithValue() noexcept{ isWithValueBuilder=true; return *this; }
        inline Builder& isRequired()  noexcept{  isRequiredBuilder=true; return *this; }
        inline Builder& withDesc(std::string desc)  noexcept{  descBuilder=desc; return *this; }
        inline Builder& withLongDesc(std::string desc)  noexcept{  longDescBuilder=desc; return *this; }
        inline Builder& withDefaultValue(std::string defaultV)  noexcept{  defaultValueBuilder=defaultV; return *this; }
        // inline Builder& WithValueSpecifier(PrintableRegex* regex)  noexcept{  regexBuilder = regex; return *this; }
        inline Builder& withValidValues(std::unordered_set<std::string>&& validValues)  noexcept{  validValuesBuilder = validValues; valueSpecifierBuilder = join(",", validValues); return *this; }
        inline Builder& withRegex(PrintableRegex regex)  noexcept{  regexBuilder = regex; return *this; }
        
        const Argument& build(){
            if(built) return *arg;
            arg.reset(new Argument(nameBuilder, isWithValueBuilder, isRequiredBuilder, descBuilder, longDescBuilder, defaultValueBuilder, valueSpecifierBuilder, std::move(validValuesBuilder), regexBuilder));
            return *arg;
        }
    };

    friend Builder;
};

struct ArgParse{

    Args::CmdLine cmd;
    // extraArguments: the application's own options, registered before parsing
    inline static void init(int argc, char ** argv, const std::vector<Argument>& extraArguments = {}){ instance = new ArgParse(argc, argv, extraArguments); }
    inline static bool isInitialized() noexcept { return instance != nullptr; }
    inline static ArgParse* getInstance(){ if(instance==nullptr) throw Args::BaseException("Before using getInstance for ArgParse you should call init(...)"); return instance; }
    inline static void cleanup(){ if(instance != nullptr) delete instance; }
    std::unordered_map<std::string, std::unordered_set<std::string>> validators;
    std::unordered_map<std::string, PrintableRegex> regexes;

    inline void addValidator(std::string key, const std::unordered_set<std::string>&& validValues){ validators[key] =  validValues ; }
    inline void addRegex(std::string key, const PrintableRegex& r){ regexes[key] =  r; }

    inline bool isDefined(std::string key) noexcept{ return cmd.isDefined("--"+key) || cmd.isDefined("-"+key) || cmd.isDefined(key);}
    inline std::string get(std::string key) noexcept{ return cmd.value("--"+key);}

    void addArgument(const Argument& a);

    inline const Args::StringList& positional() const{ return cmd.positional(); }

    std::vector<Argument> arguments;

    void checkArguments();

private:
    static ArgParse* instance;
    ArgParse(int argc, char ** argv, const std::vector<Argument>& extraArguments);
};
inline ArgParse* ArgParse::instance = nullptr;

struct UMNWeightFunction;

struct Cluster{

    Vector<const Literal*> literals;

    Cluster(const Vector<const Literal*>&& literals)noexcept: literals(literals){}

    inline std::string toString() const noexcept{ 
        std::ostringstream oss;
        oss<<std::string("Cluster(")<<literals<<std::string(")");
        return oss.str();
    }
};

int64_t meanCluster(const Cluster& cluster, UMNWeightFunction* weightFunction);

template<typename D>
struct WeightFunction: public NHash<int32_t, int64_t, D, const Literal*, std::vector<const Literal*>>{

    WeightFunction(size_t N, int64_t defaultValue = 0)noexcept: NHash<int32_t, int64_t, D, const Literal*, std::vector<const Literal*>>(N, std::move(defaultValue)), weightNone(0){}

    const int64_t& getUsingHash(const size_t& key) const override{
        if(key == CONSTANTS::INFPlus64) return weightNone;
        return NHash<int32_t, int64_t, D, const Literal*, std::vector<const Literal*>>::getUsingHash(key);
    }

private:
    const int64_t weightNone;
};


struct PerfectWeightFunction: public WeightFunction<std::vector<int64_t>>{
    
    PerfectWeightFunction(size_t N, int64_t defaultValue = 0)noexcept:  WeightFunction<std::vector<int64_t>>(N, std::move(defaultValue)){
        this->_data = new std::vector<int64_t>(this->N, defaultValue);
    }
};

struct UMNWeightFunction: public WeightFunction<std::unordered_map<size_t, int64_t>>{
    UMNWeightFunction(size_t N, int64_t defaultValue = 0)noexcept:  WeightFunction<std::unordered_map<size_t, int64_t>>(N, std::move(defaultValue)){
        this->_data = new std::unordered_map<size_t, int64_t>();
    }
};


std::ostream& operator<<(std::ostream& out, const Cluster& cluster);

template<typename D, typename A = std::vector<const Literal*>>
inline int64_t sum(WeightFunction<D>& weight,  const A& softLiterals) noexcept{
    int64_t res = 0;
    for(const Literal* literal: softLiterals){
        int64_t w = weight.at(literal->id);
        if(w > 0 && w != CONSTANTS::INFPlus64)
            NOOP;
        res += w == CONSTANTS::INFPlus64 ? 0 : w;
    }
    return res;
}

void shuffleWithinClusters(Vector<const Literal*>& targets, const std::vector<size_t>& indexes, std::mt19937& gen) noexcept;

[[nodiscard]] constexpr size_t factorial(size_t n) noexcept {
    size_t f = 1;
    for (size_t k = 2; k <= n; ++k) {
        f *= k;
    }
    return f;
}

inline void startWallTime() noexcept{ wallTime = startTimer();}
inline double getWallTime() noexcept{ return elapsedTime(wallTime);}

void cleanup() noexcept;

} // namespace fiore

#include "utility.tpp"
