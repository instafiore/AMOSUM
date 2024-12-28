#pragma once
#include <unordered_map>
#include <iostream>
#include <vector>
#include <string>
#include <regex>
#include <stdexcept>
#include <limits>

namespace SETTINGS {
const std::vector<std::string> PROPAGATORS_NAMES = {"ge_amo", "ge_eo", "le_eo"}; 
const int NONE = std::numeric_limits<int>::max() ;
const std::string TRUE_STR = "true" ;
const std::string FALSE_STR = "false" ;
const std::string PREDICATE_LB = "__lb__";
const std::string PREDICATE_UB = "__ub__";
const std::string PREDICATE_GROUP = "__group__";
const std::string PREDICATE_AUX = "__aux__";
const char SEPARATOR_ASSUMPTIONS = ':';
const char NOT = '~';
const std::string NONE_STR = "";
}