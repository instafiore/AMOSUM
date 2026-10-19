#include "utility.h"
#include "constants.h"
#include <assert.h>
#include "logger.h"
#include "stats.h"


namespace fiore {

std::ostream& operator<<(std::ostream& out, const Cluster& cluster){ return out<<cluster.toString();}

int64_t meanCluster(const Cluster& cluster, UMNWeightFunction* weight){
    int64_t sum = 0;
    const size_t &n = cluster.literals.size();
    for(const Literal* l: cluster.literals) sum += weight->at(l->id);
    return n > 0 ? sum / n : 0;
}


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
    this->addArgument(Argument::Builder(CONSTANTS::polarityHeuristic).isWithValue().withDesc("Polarity heuristic for incomplete part").withDefaultValue(CONSTANTS::defaultPolarityHeuristic).withValidValues({CONSTANTS::polarityHeuristicPol0, CONSTANTS::polarityHeuristicTORC}).build());
    this->addArgument(Argument::Builder(CONSTANTS::enableAlreadyDerivedFromAssumptionDetectionKey).isWithValue().withDesc("Whether to enable detection of soft literals Derived By Assumptions").withDefaultValue(CONSTANTS::enableAlreadyDerivedFromAssumptionDetectionDef).withValidValues({CONSTANTS::TRUE_STR, CONSTANTS::FALSE_STR}).build());
    this->addArgument(Argument::Builder(CONSTANTS::LOWER_BOUND_SHUFFLING_SELECTING_POINT_KEY).isWithValue().withDesc("Selecting the shuffling point only if "+CONSTANTS::LOWER_BOUND_SHUFFLING_KEY+" is active. A percentage p can be defined in order to select the first point where LB / best-known value >= p. Alternatively, can be selected strategies in "+Vector<std::string>({CONSTANTS::LOWER_BOUND_SHUFFLING_SELECTING_POINT_MaxIncrease}).toString()+".").withDefaultValue(CONSTANTS::LOWER_BOUND_SHUFFLING_SELECTING_POINT_Default).withRegex(Decimal01OrMaxIncrease()).build());
    for(const Argument& a: extraArguments) this->addArgument(a);
    // parsing arguments
    cmd.parse();

    this->checkArguments();
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

void shuffleWithinClusters(Vector<const Literal*>& targets, const std::vector<size_t>& indexes, std::mt19937& gen) noexcept{
    auto b = targets.begin();
    const size_t clusters = indexes.size() - 1;
    for(size_t j = 0; j < clusters; ++j){
        size_t s = indexes[j];
        size_t e = indexes[j+1];
        if(e-s == 1) continue;
        #ifdef __debug__
            const Literal* lStart = targets.get(s);
            const Literal* lEnd = e < targets.size() ? targets.get(e) : nullptr;
            LOG(LogLevel::INFO, "Shuffling between ",lStart," j: ",s,"  and ",lEnd," j: ",e);
        #endif
        std::shuffle(b+s, b+e, gen);
    }
}


void cleanup() noexcept{
    #ifndef NSTATS
        Statistics* stats = Statistics::getInstance();
    #endif
    ArgParse::cleanup();
    #ifdef __debug__
        #ifndef NSTATS
            LOG(LogLevel::ALWAYS,stats);
        #endif
        LOG(LogLevel::ALWAYS, "Exiting with exit code: ",CONSTANTS::EXIT_CODE);
        Logger::cleanup();
    #endif
    #ifndef NSTATS
        // stats->printOnSTDOUT();
        stats->printOnFile();
    #endif
}

} // namespace fiore
