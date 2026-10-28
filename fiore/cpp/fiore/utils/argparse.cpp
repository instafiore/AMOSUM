#include "utility.h"
#include "constants.h"
#include <assert.h>
#include "logger.h"
#include "stats.h"
#include "argparse.h"

namespace fiore
{

void ArgParse::addArgument(const Argument& a){
    std::string defaultValueDescString = a.defaultValue != std::string() ? std::string(" (default: "+a.defaultValue+")") : std::string();
    std::string desc = a.desc + defaultValueDescString;
    std::string longDesc = a.longDesc != std::string() ? a.longDesc : a.desc + defaultValueDescString;
    cmd.addArgWithNameOnly(a.name, a.isWithValue, a.isRequired, desc, longDesc, a.defaultValue, a.valueSpecifier);
    arguments.push_back(a);
}

// Classes

ArgParse::ArgParse(int argc, char ** argv, const std::vector<Argument>& extraArguments): cmd(argc, argv, Args::CmdLine::HandlePositionalArguments){
    cmd.addHelp(true, argv[ 0 ], "").setPositionalDescription(CONSTANTS::aspInputsKey);
    this->addArgument(Argument::Builder(CONSTANTS::TimestampToLogFileKey).withDesc("To set timestamp to log").withLongDesc("Whether to set or not the time stamp to log file").withDefaultValue(CONSTANTS::TRUE_STR).withValidValues({CONSTANTS::TRUE_STR, CONSTANTS::FALSE_STR}).build());
    this->addArgument(Argument::Builder(CONSTANTS::LogFileNameKey).isWithValue().withDesc("Path for log file").withLongDesc("The path to the file where to log").withDefaultValue(CONSTANTS::LOGFILE).build());
    this->addArgument(Argument::Builder(CONSTANTS::logKey).isWithValue().withDesc("Defining a log level").withLongDesc("The log level to be used").withDefaultValue(Logger::levelToString(LogLevel::INFO)).withValidValues(getLogLevelNames()).build());
    this->addArgument(Argument::Builder(CONSTANTS::STATSFILEKey).isWithValue().withDesc("Defining a stats file").withLongDesc("Path to stats file").withDefaultValue(CONSTANTS::STATSFILEDefault).build());
    this->addArgument(Argument::Builder(CONSTANTS::seedKey).isWithValue().withDesc("Seed").withDefaultValue(CONSTANTS::DefaultSeed).withRegex(UnsignedInteger()).build());
    this->addArgument(Argument::Builder(CONSTANTS::verboseKey).isWithValue().withDesc("1: cost and time per answer set\n2: cost, time, and answer set").withDefaultValue(CONSTANTS::verboseDef).withValidValues({CONSTANTS::verboseLevel1, CONSTANTS::verboseLevel2}).build());
    for(const Argument& a: extraArguments) this->addArgument(a);
    // parsing arguments
    build();
}

void ArgParse::checkArguments(){
       
        for(const Argument& a: arguments){
            std::string key = a.name;
            const std::unordered_set<std::string> possibleValue = a.validValues;
            bool defined = this->isDefined(key) && possibleValue.size() > 0;
            if(defined){
                std::string value = this->get(key);
                if(possibleValue.find(value) == possibleValue.end()){
                    std::ostringstream out;
                    out<<"Value: "<<value<<" for argument: "<<key<<"  not accepted. Possible values are: "<<possibleValue;
                    throw Args::BaseException(out.str());
                }
            }

            const PrintableRegex &regex = a.regex;
            defined = this->isDefined(key) && regex != PrintableRegex();
            if(defined){
                std::string value = this->get(key);
                if(!std::regex_match(value,regex)){
                    std::ostringstream out;
                    out<<"Value: "<<value<<" for key: "<<key<<"  not accepted, regex: "<<regex;
                    throw Args::BaseException(out.str());
                }
            }
            
        }
}  


} // namespace fiore

