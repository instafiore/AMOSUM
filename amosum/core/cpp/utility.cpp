#include "amosum/core/cpp/utility.h"
#include <cstdio>
#include <cstdlib>
#include <sstream>

std::vector<std::string> split(const std::string& str, char delimiter) {
    std::vector<std::string> tokens;
    std::istringstream stream(str);
    std::string token;
    while (std::getline(stream, token, delimiter)) tokens.push_back(token);
    return tokens;
}

std::string from_symbol_to_string(clingo_symbol_t symbol){
    size_t symbol_size;
    handle_error(clingo_symbol_to_string_size(symbol, &symbol_size));
    std::vector<char> symbol_str_c(symbol_size);
    handle_error(clingo_symbol_to_string(symbol, symbol_str_c.data(), symbol_size));
    return std::string(symbol_str_c.data());
}

namespace {
    const char* clingo_error_code_to_string(clingo_error_t code) {
        switch (code) {
            case clingo_error_bad_alloc: return "memory could not be allocated";
            case clingo_error_unknown:   return "errors unrelated to clingo";
            case clingo_error_success:   return "successful API calls";
            case clingo_error_runtime:   return "errors only detectable at runtime like invalid input";
            case clingo_error_logic:     return "wrong usage of the clingo API";
            default:                     return "unknown_error code: ";
        }
    }
}

void handle_error(bool success) {
    if (success) return;
    const char* error_message = clingo_error_message();
    if (error_message == nullptr) error_message = "generic error, not recognized from clingo";
    const clingo_error_t code = clingo_error_code();
    fprintf(stderr, "Clingo error: %s with code: %s\n", error_message, clingo_error_code_to_string(code));
    exit(code);
}
