#pragma once
#include "propagator_clingo_initializer.h"


void PropagatorClingoInitializer::init(clingo_propagate_init* _init, PropagatorClingo& propagator){

    if(!first){
        propagator.atomNames = atomNames;
        propagator.map_slit_plit = map_slit_plit.get();
        propagator.map_plit_slit = map_plit_slit.get();
        return ;
    }
    first = false ;

    atomNames = new std::unordered_map<clingo_symbol_t, clingo_literal_t>();
 
    map_slit_plit.reset(new std::unordered_map<clingo_literal_t, std::vector<clingo_literal_t>>());
    map_plit_slit.reset(new std::unordered_map<clingo_literal_t, clingo_literal_t>());
    nt = clingo_propagate_init_number_of_threads(_init);

    debug("[init] number of threads ", nt);
    clingo_symbolic_atoms_t const *symbolic_atoms;
    clingo_symbolic_atom_iterator_t symbolic_atoms_it, symbolic_atoms_ie;
    
    handle_error(clingo_propagate_init_symbolic_atoms(_init, &symbolic_atoms));
    handle_error(clingo_symbolic_atoms_end(symbolic_atoms, &symbolic_atoms_ie));

    handle_error(clingo_symbolic_atoms_begin(symbolic_atoms, NULL, &symbolic_atoms_it));

    while (true) {
        bool equal;
        clingo_literal_t plit;
        clingo_literal_t slit;
        clingo_symbol_t symbol;

        // stop iteration if the end is reached
        handle_error(clingo_symbolic_atoms_iterator_is_equal_to(symbolic_atoms, symbolic_atoms_it, symbolic_atoms_ie, &equal));
        if (equal) { break; }
        handle_error(clingo_symbolic_atoms_symbol(symbolic_atoms, symbolic_atoms_it, &symbol));
        std::string symbol_str = from_symbol_to_string(symbol) ;

        
        handle_error(clingo_symbolic_atoms_literal(symbolic_atoms, symbolic_atoms_it, &plit));
        handle_error(clingo_propagate_init_solver_literal(_init, plit, &slit));

        if (plit > max_plit) { max_plit = plit; }

        // debugf("[init] symbol: ", symbol_str, " symbol: ", symbol, " plit: ", plit, " slit: ", slit)

        atomNames->emplace(symbol, plit);
        (*map_plit_slit)[plit] = slit ; 
        (*map_plit_slit)[not_(plit)] = not_(slit) ; 
        update_map_value_vector<clingo_literal_t, clingo_literal_t>(*map_slit_plit, slit, plit);
        update_map_value_vector<clingo_literal_t, clingo_literal_t>(*map_slit_plit, not_(slit), not_(plit));
    
        clingo_symbolic_atoms_next(symbolic_atoms, symbolic_atoms_it, &symbolic_atoms_it);
    }
    
    lits = new std::vector<clingo_literal_t>{max_plit};

    std::vector<clingo_literal_t> facts = get_map_value_vector<clingo_literal_t, clingo_literal_t>((*map_slit_plit), 1);
    extend_vector(*lits, facts);

    propagator.atomNames = atomNames;
    propagator.map_slit_plit = map_slit_plit.get();
    propagator.map_plit_slit = map_plit_slit.get();
}

PropagatorClingoInitializer* PropagatorClingoInitializer::instance = nullptr;