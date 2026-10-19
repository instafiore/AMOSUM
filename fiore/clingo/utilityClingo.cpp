#include "utilityClingo.h"
#include <sstream>
#include <ostream>
#include "../utils/logger.h"
#include "../utils/utilityCommon.h"
#include "../utils/utility.h"


namespace fiore::clingo {

const std::vector<clingo_literal_t> EMPTY_LITERALS{};

std::ostream& operator<<(std::ostream& out, const ClingoResult& clingoResult){ return out<<clingoResult.toString();}


void clingoLogger(clingo_warning_t code, char const *message, void *data) {
    std::stringstream ss;
    ss << "Clingo warning [" << code << "]: " << message << std::endl;
    #ifdef __debug__
        LOG(LogLevel::CLINGO_WARNING, ss.str());
    #else
        std::cerr<<ss.str();
    #endif
}

std::string from_symbol_to_string(clingo_symbol_t symbol) noexcept {
    size_t symbol_size;
    handleError(clingo_symbol_to_string_size(symbol, &symbol_size));
    char symbol_str_c[symbol_size]; 
    handleError(clingo_symbol_to_string(symbol, symbol_str_c, symbol_size));
    std::string symbol_str = std::string(symbol_str_c);
    return symbol_str ;
}


const std::string clingo_error_code_to_string(clingo_error_t code) {
    switch (code) {
        case clingo_error_bad_alloc:
            return "memory could not be allocated";
        case clingo_error_unknown:
            return "errors unrelated to clingo";
        case clingo_error_success:
            return "successful API calls";
        case clingo_error_runtime:
            return "errors only detectable at runtime like invalid input";
        case clingo_error_logic:
            return "wrong usage of the clingo API";
        default:
            return "unknown_error code: ";
    }
}

void handle_error(bool success, const char* file, size_t line) {
    char const *error_message;
    if (!success) {
        if (!(error_message = clingo_error_message())) { error_message = "generic error, not recognized from clingo"; }
        clingo_error_t code = clingo_error_code();
        std::string code_str = clingo_error_code_to_string(code);
        std::stringstream ss;
        ss << "Clingo error: " << error_message << " with code: " << code_str << " at " << file << ":" << line << '\n';
        LOG(LogLevel::ERROR, ss.str());
    }
}

#ifdef __debug__

bool printConfigurationClingo(const clingo_configuration_t *conf, clingo_id_t key, int depth) {

    bool ret = true;
    clingo_configuration_type_bitset_t type;

    // 1. Get the type of the current node
    if (!clingo_configuration_type(conf, key, &type)) { goto error; }

    // 2. Check if it is a Value (Leaf)
    if (type & clingo_configuration_type_value) {
        
        // Configuration values are always returned as strings
        size_t size;
        clingo_configuration_value_get_size(conf, key, &size);
        char value[size];
        if (!clingo_configuration_value_get(conf, key, value, size)) { goto error; }
        
        // Value might be NULL if the option has no default/current value
        if (size > 0) {
            LOG(LogLevel::INFO_1, indent(depth), "value ", std::string(value));
        } else {
            LOG(LogLevel::INFO_1, indent(depth), "value ", "(empty)");
        }
    }

    // 3. Check if it is an Array
    else if (type & clingo_configuration_type_array) {
        size_t size;
        if (!clingo_configuration_array_size(conf, key, &size)) { goto error; }
        
        for (size_t i = 0; i < size; ++i) {
            clingo_id_t subkey;
            // Get the subkey for the item at index i
            if (!clingo_configuration_array_at(conf, key, i, &subkey)) { goto error; }

            LOG(LogLevel::INFO_1, indent(depth), "array ", i);

            // Recursively print subentry
            if (!printConfigurationClingo(conf, subkey, depth + 1)) { goto error; }
        }
    }

    // 4. Check if it is a Map
    else if (type & clingo_configuration_type_map) {
        size_t size;
        if (!clingo_configuration_map_size(conf, key, &size)) { goto error; }

        for (size_t i = 0; i < size; ++i) {
            char const *name;
            clingo_id_t subkey;

            // Get the name of the key at index i
            if (!clingo_configuration_map_subkey_name(conf, key, i, &name)) { goto error; }
            // Get the actual subkey ID using that name
            if (!clingo_configuration_map_at(conf, key, name, &subkey)) { goto error; }

            LOG(LogLevel::INFO_1, indent(depth), "map ", name);

            // Recursively print subentry
            if (!printConfigurationClingo(conf, subkey, depth + 1)) { goto error; }
        }
    }

    goto out;

error:
    ret = false;
out:
    return ret;
}


bool printClingoStatisticClingo(const clingo_statistics_t *stats, uint64_t key, int depth) {

    bool ret = true;
    clingo_statistics_type_t type;
   
    if (!clingo_statistics_type(stats, key, &type)) { goto error; }
    switch ((enum clingo_statistics_type_e)type) {

      case clingo_statistics_type_value: {
        double value;
        if (!clingo_statistics_value_get(stats, key, &value)) { goto error; }
        LOG(LogLevel::INFO_1, indent(depth),"value ",  value);
        break;
      }
   
      case clingo_statistics_type_array: {
        size_t size;
   
        if (!clingo_statistics_array_size(stats, key, &size)) { goto error; }
        for (size_t i = 0; i < size; ++i) {
          uint64_t subkey;
          if (!clingo_statistics_array_at(stats, key, i, &subkey)) { goto error; }
        LOG(LogLevel::INFO_1,indent(depth), "array ", i);
   
          // recursively print subentry
          if (!printClingoStatisticClingo(stats, subkey, depth+1)) { goto error; }
        }
        break;
      }
   
      // print maps
      case clingo_statistics_type_map: {
        size_t size;
   
        // loop over map elements
        handleError(clingo_statistics_map_size(stats, key, &size));
        for (size_t i = 0; i < size; ++i) {
          char const *name;
          uint64_t subkey;
   
            if (!clingo_statistics_map_subkey_name(stats, key, i, &name)) { goto error; }
            if (!clingo_statistics_map_at(stats, key, name, &subkey)) { goto error; }
            LOG(LogLevel::INFO_1,indent(depth), "map ", name);
            if (!printClingoStatisticClingo(stats, subkey, depth+1)) { goto error; }
        }
      }
   
      case clingo_statistics_type_empty: { goto out; }
    }
   
    goto out;

  error:
    ret = false;
  out:

    return ret;
}

#endif

bool getMapKeyStatisticClingo(const clingo_statistics_t *stats, uint64_t parent_key, std::string name, uint64_t *out_key) {
    bool has_key;
    handleError(clingo_statistics_map_has_subkey(stats, parent_key, name.c_str(), &has_key));
    if (!has_key) return false;
    bool res = clingo_statistics_map_at(stats, parent_key, name.c_str(), out_key);
    return res;
}


double getValueStatisticClingo(const clingo_statistics_t *stats, std::string path){

    std::vector<std::string> tokens = split(path, '/');
    uint64_t key;
    double value;

    handleError(clingo_statistics_root(stats, &key));

    for(size_t i = 0; i < tokens.size(); ++i){
        std::string &token = tokens[i];
        getMapKeyStatisticClingo(stats, key, token, &key);
    }
    
    clingo_statistics_value_get(stats, key, &value);

    return value;
}


void setVSIDSHeuristic(clingo_control_t* control){
    clingo_configuration_t *conf;
    clingo_control_configuration(control, &conf);

    clingo_id_t root;
    clingo_configuration_root(conf, &root);

    /* solver */
    clingo_id_t solver;
    clingo_configuration_map_at(conf, root, "solver", &solver);

    /* solver is an array → take index 0 */
    clingo_id_t solver0;
    clingo_configuration_array_at(conf, solver, 0, &solver0);

    /* heuristic inside solver[0] */
    clingo_id_t heuristic;
    clingo_configuration_map_at(conf, solver0, "heuristic", &heuristic);

    /* set VSIDS */
    clingo_configuration_value_set(conf, heuristic, "vsids,92");

}

std::unordered_set<std::string> statKeys(const clingo_statistics_t *stats, uint64_t key){
    size_t size;
    // loop over map elements
    handleError(clingo_statistics_map_size(stats, key, &size));
    std::unordered_set<std::string> keys;
    for (size_t i = 0; i < size; ++i) {
        char const *name;
        uint64_t subkey;

        handleError(clingo_statistics_map_subkey_name(stats, key, i, &name));
        handleError(clingo_statistics_map_at(stats, key, name, &subkey));
        keys.emplace(name);
    }
    return keys;
}







std::unordered_set<std::string> configurationKeys(const clingo_configuration_t *conf, clingo_id_t key){
    size_t size;
    // loop over map elements
    handleError(clingo_configuration_map_size(conf, key, &size));
    std::unordered_set<std::string> keys;
    for (size_t i = 0; i < size; ++i) {
        char const *name;
        clingo_id_t subkey;
        handleError(clingo_configuration_map_subkey_name(conf, key, i, &name));
        handleError(clingo_configuration_map_at(conf, key, name, &subkey));
        keys.emplace(name);
    }
    return keys;
}


bool getMapKeyConfiguration(const clingo_configuration_t* conf, clingo_id_t parent_key, std::string name, clingo_id_t *out_key) {
    bool has_key;
    handleError(clingo_configuration_map_has_subkey(conf, parent_key, name.c_str(), &has_key));
    if (!has_key) return false;
    bool res = clingo_configuration_map_at(conf, parent_key, name.c_str(), out_key);
    return res;
}

void setValueConfigurationClingo(clingo_configuration_t *conf, std::string path, std::string value){

    std::vector<std::string> tokens = split(path, '/');
    clingo_id_t key;

    clingo_configuration_root(conf, &key);

    // printConfigurationClingo(conf, key);
    // exit(0);


    for(size_t i = 0; i < tokens.size(); ++i){
        std::string &token = tokens[i];
        if(!getMapKeyConfiguration(conf, key, token, &key)){
            #ifdef __debug__ 
                std::string helpStr;
                clingo_configuration_type_bitset_t type;
                handleError(clingo_configuration_type(conf, key, &type));
                switch ((enum clingo_configuration_type_e)type) {
                    case clingo_configuration_type_value:
                        LOG(LogLevel::ERROR, "Key: ",token, " because it is a value !");
                        break;
                    case clingo_configuration_type_map:
                        LOG(LogLevel::ERROR, "Key: ",token, " not present in configuration path: ",path, " possible keys are: ", configurationKeys(conf, key));
                        break; 
                    case clingo_configuration_type_array:
                        LOG(LogLevel::ERROR, "Key: ",token, " because it is an array !");
                        break; 
                    default:
                        LOG(LogLevel::ERROR, "Key: ",token, " not recognized!");
                        break; 
                }
                
            #endif
        }
    }

    handleError(clingo_configuration_value_set(conf, key, value.c_str()));

    #ifdef __debug__
        LOG(LogLevel::INFO,"Configuration update: ", path," set to: ", value);
    #endif

}

} // namespace fiore::clingo
