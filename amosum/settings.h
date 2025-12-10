#pragma once
#include <unordered_map>
#include <iostream>
#include <vector>
#include <string>
#include <regex>
#include <stdexcept>
#include <limits>

namespace SETTINGS {
const std::vector<std::string> PROPAGATORS_NAMES = {"ge_amo", "ge_eo", "le_eo","amomaximize"}; 
const int NONE = std::numeric_limits<int>::max() ;
const std::string TRUE_STR = "true" ;
const std::string FALSE_STR = "false" ;
const std::string PREDICATE_LB = "__lb__";
const std::string PREDICATE_UB = "__ub__";
const std::string PREDICATE_GROUP = "__group__";
const std::string PREDICATE_AUX = "__aux__";
const std::string REGEX_AUX = PREDICATE_AUX + "\\([\\w\\s,\\(\\)]+,\\s*(ge_amo|ge_eo|le_eo)\\)";
const char SEPARATOR_ASSUMPTIONS = ':';
const char NOT = '~';
const std::string NONE_STR = "";
const std::string LAZY_HYBRID = "hybrid";
const int SAT = 10;
const int UNSAT = 20;
const int UNKNOWN = 29;
const int OPTIMUM = 30;
const int SEARCH_SPACE_EXHAUSTED = 40;
const int TIMEOUT = 60;
const int ERROR = 70;
}
