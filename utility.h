#pragma once

#include <unordered_map>
#include <iostream>
#include "settings.h"
#include <clingo.h>
#include <sstream>
#include <cassert>

class PropagatorClingo ;
class InterpretationFunction;
class AggregateFunction;
class GroupFunction;
// Macro to handle the debug functionality

// Helper variadic template function
template<typename... Args>
void debug_print(const Args&... args) {
    std::ostringstream oss;
    (oss << ... << args); 
    std::cerr << oss.str() << std::endl;
}

template<typename... Args>
void print(const Args&... args) {
    std::ostringstream oss;
    (oss << ... << args); 
    std::cout << oss.str() << std::endl;
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
template <typename Key, typename Value>
std::string unordered_map_to_string(std::unordered_map<Key, Value> map);
std::vector<std::string> split(const std::string& str, char delimiter);
template <typename T>
std::string vector_to_string(const std::vector<T>& vec);
std::string cat(const std::string &filename);
std::string from_symbol_to_string(clingo_symbol_t sym);
clingo_symbol_t from_string_to_symbol(std::string str, const std::unordered_map<clingo_symbol_t, clingo_literal_t> &atomNames);
clingo_literal_t from_string_to_lit(std::string str, const std::unordered_map<clingo_symbol_t, clingo_literal_t> &atomNames);
int64_t from_string_to_symbol_or_lit(std::string str, const std::unordered_map<clingo_symbol_t, clingo_literal_t> &atomNames, bool sym);
void handle_error(bool success);
bool print_model(clingo_model_t const *model);
bool solve(clingo_control_t *ctl, clingo_solve_result_bitset_t *result);

struct AmoSumPropagator;

class Group {
public:
    static int autoincrement;  // Static counter for automatic ID assignment

    // Member variables
    int N;                         // Number of literals
    int count_undef;               // Number of undefined literals
    std::vector<int> ord_l;        // Ordered literals by weight
    std::vector<int> ord_l_origin; // Original order of literals
    std::unordered_map<int, int> ord_i;      // Inverse map for ord_l (literal -> index in ord_l)
    int max_und;                   // Index of the maximum undefined literal in ord_l
    int min_und;                   // Index of the minimum undefined literal in ord_l
    std::vector<int> falses_facts; // All false fact literals of the group
    std::string id;                        // Group ID
    int id_autoinc;                // Auto-incremented ID for the group

    // Constructor
    Group(std::vector<int> ord_l, std::unordered_map<int, int> ord_i, std::string id)
        : ord_l(std::move(ord_l)), ord_i(std::move(ord_i)), id(id), id_autoinc(autoincrement++) {
        N = this->ord_l.size();
        count_undef = N;
        ord_l_origin = this->ord_l;  // Copy original order
        max_und = N - 1;
        min_und = 0;
    }

    // Methods for increasing and decreasing undefined literals count
    void increase_und() { count_undef++; }
    void decrease_und() { count_undef--; }

    // Methods for handling false literals
    void add_false_lit(int lit) { falses_facts.push_back(lit); }
    void remove_false_lit(int lit) { 
        falses_facts.erase(std::remove(falses_facts.begin(), falses_facts.end(), lit), falses_facts.end());
    }

    // Setting max and min undefined literals
    void set_max(int l) { max_und = (l != -1) ? ord_i[l] : -1; }
    void set_min(int l) { min_und = (l != -1) ? ord_i[l] : -1; }

    void set_max_min(int l, bool max) {
        if (max) set_max(l);
        else set_min(l);
    }

    // Getting the most undefined literal
    int get_most_undefined(bool max);

    // Update max or min undefined literal based on some condition
    std::pair<clingo_literal_t, clingo_literal_t> update_max(const std::unique_ptr<InterpretationFunction>& I, bool all, bool update, const clingo_literal_t& assuming_und);
    std::pair<clingo_literal_t, clingo_literal_t> update_min(const std::unique_ptr<InterpretationFunction>& I, bool all , bool update, const clingo_literal_t& assuming_und);
    std::pair<clingo_literal_t, clingo_literal_t> update(const std::unique_ptr<InterpretationFunction>& I, bool max, bool all, bool update, const clingo_literal_t& assuming_und);

    // Print the group
    void print_group(const std::unordered_map<int, std::string>& atomNames) const;

    // Overloaded stream operator for printing
    friend std::ostream& operator<<(std::ostream& os, const Group& g) {
        os << g.id;
        return os;
    }

    bool operator==(const Group& other) const {
        return id == other.id;
    }
};



template <typename V>
class PerfectHash {
public:
    // Constructor that initializes the hash table
    PerfectHash(int N, V default_value = V()): N(N), values(2*N,default_value){}

    // Getter for index access
    virtual V get(int key);

    // Setter for index access
    virtual void set(const int &key, const V& value);

protected:
    std::vector<V> values;  
private:
    int N;                 
};

class TrueGroupFunction : private PerfectHash<clingo_literal_t> {
public:
    TrueGroupFunction(int N, int default_value = SETTINGS::NONE) : PerfectHash(N, default_value) {}

    // Overriding the setter to use group ID as the key
    void set(const Group* key, const clingo_literal_t &value) {
        int autoincrement = key->id_autoinc;
        this->values[autoincrement] = value;
    }

    // Overriding the getter to use group ID as the key
    clingo_literal_t get(const Group* group) const {
        int autoincrement = group->id_autoinc;
        return this->values[autoincrement];
    }
};

inline int not_(int literal) { return -literal; }

std::vector<clingo_literal_t>* create_reason_falses(const AmoSumPropagator& propagator, bool ge);
std::vector<clingo_literal_t>* create_reason_falses_ge(const AmoSumPropagator& propagator);
std::vector<clingo_literal_t>* create_reason_falses_le(const AmoSumPropagator& propagator);
std::tuple<bool, const std::vector<clingo_literal_t>* (*)(const Group &G, AmoSumPropagator &propagator), std::string>  get_propagator_variables(std::string prop_type);
// Function to get the name
std::string get_name(const std::unordered_map<clingo_symbol_t, clingo_literal_t>& atomNames, clingo_literal_t lit);
void print_propagate(PropagatorClingo* prop, const clingo_literal_t *changes, size_t size, clingo_propagate_control_t *control, int dl, bool force_print, bool wasp_b);
std::string atomNames_to_string(std::unordered_map<clingo_symbol_t, clingo_literal_t> atomNames);
std::vector<std::string> convert_assparam_to_assarray(const std::string& assumptions);
std::vector<clingo_literal_t> create_assumptions_lits(const std::vector<std::string>& assumptions_vec, const std::unordered_map<clingo_symbol_t, clingo_literal_t>& atomNames);


clingo_literal_t max_w(const Group* g);
// Function to return the min undefined literal
clingo_literal_t min_w(const Group* g);
// Function to select between max_w and min_w
clingo_literal_t m_w(const Group* g, bool max);
bool equals(const clingo_literal_t& l1, const clingo_literal_t& l2);
    

struct Minimize {
    static constexpr const char* NO_MINIMIZATION = "default" ;
    static constexpr const char* MINIMAL = "min" ;
    static constexpr const char* CARDINALITY_MINIMAL = "cmin" ;
};

void simplifyLiterals(std::vector<clingo_literal_t>& lits, AggregateFunction* aggregate, GroupFunction* group, bool max, const std::unique_ptr<InterpretationFunction>& I); 
#include "utility.tpp"  // Include the template implementation file

