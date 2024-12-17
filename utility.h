#pragma once

#include <unordered_map>
#include <iostream>
#include "settings.h"
#include <clingo.h>
#include <sstream>

// Macro to handle the debug functionality

// Helper variadic template function
template<typename... Args>
void debug_print(const Args&... args) {
    std::ostringstream oss;
    (oss << ... << args); 
    std::cerr << oss.str() << std::endl;
}

#ifdef DEBUG
    #define debugf(...) \
        debug_print(__VA_ARGS__); \
    
    #define debug(...) \
        if ((DEBUG)) { \
            debug_print(__VA_ARGS__); \
        }
#else
    #define debugf(...)
    #define debug(...)
#endif

using ParameterMap = std::unordered_map<std::string, std::string>;


std::unordered_map<std::string, std::string> init_param(int argc, char const *argv[]);
void print_unordered_map(std::unordered_map<std::string, std::string> map);
std::vector<std::pair<std::string, ParameterMap>> process_sys_parameters(const std::vector<std::string>& sys_parameters);
std::string unordered_map_to_string(std::unordered_map<std::string, std::string> map);
std::vector<std::string> split(const std::string& str, char delimiter);
template <typename T>
std::string vector_to_string(const std::vector<T>& vec);
std::string cat(const std::string &filename);

void handle_error(bool success);
bool print_model(clingo_model_t const *model);
bool get_arg(clingo_symbol_t sym, int offset, int *num);
bool solve(clingo_control_t *ctl, clingo_solve_result_bitset_t *result);