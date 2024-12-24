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
}