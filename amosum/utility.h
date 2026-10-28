#pragma once
#include <string>
#include <vector>
#include <clingo.h>
#include "settings.h"
#include "fiore/utils/utilityCommon.h"

// helpers of amosum on top of fiore

// str split at every delimiter
std::vector<std::string> split(const std::string& str, char delimiter);

// the text of a clingo symbol
std::string from_symbol_to_string(clingo_symbol_t symbol);

// exits with clingo's error code (and its message on stderr) when a clingo call fails
void handle_error(bool success);

inline int not_(int literal) {
    if(literal == SETTINGS::NONE) return SETTINGS::NONE;
    return -literal;
}
