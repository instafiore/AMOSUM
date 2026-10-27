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

namespace fiore
{

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
    inline void build() { cmd.parse(); this->checkArguments();}

protected:
    static ArgParse* instance;
    ArgParse(int argc, char ** argv, const std::vector<Argument>& extraArguments);
};
inline ArgParse* ArgParse::instance = nullptr;
    
} // namespace fiore


