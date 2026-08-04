#include "common_utility.h"



// Functions

size_t std::hash<cmn::Mapping2Integer>::operator()(const cmn::Mapping2Integer& obj) const noexcept {
    return std::hash<int32_t>{}(obj.integer());
}

std::ostream& operator<<(std::ostream& out, const char* value) {
    if(value) out << std::string(value);
    else out << std::string("");
    return out;
}

std::ostream& operator<<(std::ostream& out, const cmn::Literal& literal){ return out<<literal.toString(); }
std::ostream& operator<<(std::ostream& out, const cmn::ClingoResult& clingoResult){ return out<<clingoResult.toString();}
std::ostream& operator<<(std::ostream& out, const cmn::TruthValue& truthValue){return out<<truthValue.toString();}
std::ostream& operator<<(std::ostream& out, const cmn::PrintableRegex& p){ return out<<p.input; }
std::ostream& operator<<(std::ostream& out, const cmn::MapLiteralId2Literal map){ return out<<map.toString();}

// template std::ostream& operator<< <Literal*, Hasher>(std::ostream&, const PerfectVector<Literal*,Hasher>&);

// size_t std::hash<Mapping2Integer*>::operator()(const Mapping2Integer* obj) const noexcept {
//     return std::hash<Mapping2Integer>{}(*obj);
// }

void cmn::setExitCode(int exitCode, bool exitOnError) noexcept {
    assert(CONSTANTS::EXIT_CODE == CONSTANTS::NOERROR_CODE || CONSTANTS::EXIT_CODE == exitCode);
    CONSTANTS::EXIT_CODE = exitCode ;
    if(CONSTANTS::EXIT_CODE == CONSTANTS::ERROR_CODE){
        if(exitOnError) exit(CONSTANTS::ERROR_CODE);
    };
}

char const *const * cmn::fromVecStr2VecCstring(std::vector<std::string>& vecString){ 
    char const ** res = new const char*[vecString.size()];
    for(size_t i = 0; i < vecString.size(); ++i){
        const char * cstr = vecString[i].c_str();
        res[i] = cstr;
    }
    return res;
}



inline std::string cmn::MapLiteralId2Literal::toString() const{ 
    std::ostringstream oss; ::operator<<(oss,map) ; return oss.str();
}

void cmn::addArgControl(std::vector<std::string>& arguments, std::string key, std::string value) noexcept{
    std::string valueStr = value.size() != 0 ? "="+value : "";
    std::string arg = "--"+key+valueStr;
    arguments.push_back(arg);
}
