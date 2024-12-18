#include <clingo.h>
#include <stdlib.h>
#include <stdio.h>
#include <string>
#include <assert.h>
#include "../../utility.h"
#include "../../amosum.h"
#include <sstream>
#include <iostream>
#include <vector>
#include <limits>
#include "propagator_clingo.h"

bool PropagatorClingo::init(clingo_propagate_init_t *init){
    size_t threads = clingo_propagate_init_number_of_threads(init);
    debug("[init] number of threads ", threads)

    clingo_literal_t max_plit = 0;
    clingo_symbolic_atoms_t const *symbolic_atoms;
    clingo_symbolic_atom_iterator_t symbolic_atoms_it, symbolic_atoms_ie;
    
    handle_error(clingo_propagate_init_symbolic_atoms(init, &symbolic_atoms));
    handle_error(clingo_symbolic_atoms_end(symbolic_atoms, &symbolic_atoms_ie));

    handle_error(clingo_symbolic_atoms_begin(symbolic_atoms, NULL, &symbolic_atoms_it));


    std::vector<std::tuple<std::string, clingo_literal_t, clingo_literal_t>> atoms_list_for_mapping;
     
    while (true) {
        bool equal;
        clingo_literal_t plit;
        clingo_literal_t slit;
        clingo_symbol_t symbol;
 
        // stop iteration if the end is reached
        handle_error(clingo_symbolic_atoms_iterator_is_equal_to(symbolic_atoms, symbolic_atoms_it, symbolic_atoms_ie, &equal));
        if (equal) { break; }
        handle_error(clingo_symbolic_atoms_symbol(symbolic_atoms, symbolic_atoms_it, &symbol));
        size_t symbol_size;
        handle_error(clingo_symbol_to_string_size(symbol, &symbol_size));
        char symbol_str_c[symbol_size]; 
        handle_error(clingo_symbol_to_string(symbol, symbol_str_c, symbol_size));
        std::string symbol_str = std::string(symbol_str_c);
        
        handle_error(clingo_symbolic_atoms_literal(symbolic_atoms, symbolic_atoms_it, &plit));
        handle_error(clingo_propagate_init_solver_literal(init, plit, &slit));

        if (plit > max_plit) { max_plit = plit; }

        debug("[init] symbol: ", symbol_str, " plit: ", plit, " slit: ", slit)
        atoms_list_for_mapping.emplace_back(symbol_str, plit, slit);

      
        clingo_symbolic_atoms_next(symbolic_atoms, symbolic_atoms_it, &symbolic_atoms_it);
    }

    debug("max plit: ", max_plit);

    for(const auto& tuple: atoms_list_for_mapping){
        this->atomNames.emplace(std::get<0>(tuple), std::get<1>(tuple));
    }

    debug("atomNames: ", unordered_map_to_string(atomNames));

    return true ;
}

 


