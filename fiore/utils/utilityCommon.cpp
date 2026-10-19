#include "utilityCommon.h"
// #include "logger.h"
#include "utility.h"
// Functions





namespace fiore {

std::ostream& operator<<(std::ostream& out, const char* value) {
    if(value) out << std::string(value);
    else out << std::string("");
    return out;
}

std::ostream& operator<<(std::ostream& out, const Literal& literal){ return out<<literal.toString(); }
std::ostream& operator<<(std::ostream& out, const TruthValue& truthValue){return out<<truthValue.toString();}
std::ostream& operator<<(std::ostream& out, const PrintableRegex& p){ return out<<p.input; }
std::ostream& operator<<(std::ostream& out, const MapLiteralId2Literal map){ return out<<map.toString();}
// template std::ostream& operator<< <Literal*, Hasher>(std::ostream&, const PerfectVector<Literal*,Hasher>&);

} // namespace fiore

size_t std::hash<fiore::Mapping2Integer>::operator()(const fiore::Mapping2Integer& obj) const noexcept {
    return std::hash<int32_t>{}(obj.integer());
}

namespace fiore {

// size_t std::hash<Mapping2Integer*>::operator()(const Mapping2Integer* obj) const noexcept {
//     return std::hash<Mapping2Integer>{}(*obj);
// }

void setExitCode(int exitCode, bool exitOnError, std::string message) noexcept {
    assert(CONSTANTS::EXIT_CODE == CONSTANTS::NOERROR_CODE || CONSTANTS::EXIT_CODE == exitCode);
    CONSTANTS::EXIT_CODE = exitCode ;
    printf("%s", message.c_str());
    if(CONSTANTS::EXIT_CODE == CONSTANTS::ERROR_CODE){
        if(exitOnError) exit(CONSTANTS::ERROR_CODE);
    };
}

double random01(const size_t& seed) {
    static std::mt19937 gen(seed);
    static std::uniform_real_distribution<double> dist(0.0, 1.0);
    return dist(gen);
}

bool randomBool(const size_t& seed, float p) {
    if(p == 1 || p == 0) return p;
    static std::mt19937 gen(std::stoi(ArgParse::getInstance()->get(CONSTANTS::seedKey)));
    static std::bernoulli_distribution dist(p);
    return dist(gen);
}

char const *const * fromVecStr2VecCstring(std::vector<std::string>& vecString){ 
    char const ** res = new const char*[vecString.size()];
    for(size_t i = 0; i < vecString.size(); ++i){
        const char * cstr = vecString[i].c_str();
        res[i] = cstr;
    }
    return res;
}



template<typename V, typename Checker, typename Indexer>
inline IterableSet<const V*, Checker, Indexer>* copyIterableSet(const IterableSet<std::unique_ptr<V>, Checker, Indexer>& vec) noexcept{
    IterableSet<const V*, Checker, Indexer>* vecNew = new IterableSet<const V*, Checker, Indexer>(vec.N);
    for(const std::unique_ptr<V>& elem: vec) vecNew->add(elem.get());
    return vecNew;
}

inline std::string MapLiteralId2Literal::toString() const{ 
    std::ostringstream oss; oss<<map; return oss.str();
}

} // namespace fiore
