from typing import Dict, List, Optional
from clingo import Symbol, PropagateInit, SymbolicAtoms
import clingo

class PropagatorClingoInitializer:
    _instance: Optional["PropagatorClingoInitializer"] = None

    def __init__(self):
        if PropagatorClingoInitializer._instance is not None:
            raise Exception("This class is a singleton!")
        self.first = True
        self.atomNames: Optional[Dict[str, int]] = None
        self.map_slit_plit: Optional[Dict[int, List[int]]] = None
        self.map_plit_slit: Optional[Dict[int, int]] = None
        self.nt = 0
        self.max_plit = 0
        self.lits: Optional[List[int]] = None
        PropagatorClingoInitializer._instance = self

    @staticmethod
    def get_instance():
        if PropagatorClingoInitializer._instance is None:
            PropagatorClingoInitializer._instance = PropagatorClingoInitializer()
        return PropagatorClingoInitializer._instance

    def init(self, _init: clingo.PropagateInit, propagator):
        if not self.first:
            propagator.atomNames = self.atomNames
            propagator.map_slit_plit = self.map_slit_plit
            propagator.map_plit_slit = self.map_plit_slit
            return
        
        self.first = False

        self.atomNames = {}
        self.map_slit_plit = {}
        self.map_plit_slit = {}
        self.nt = _init.number_of_threads
        print(f"[init] number of threads: {self.nt}")

        symbolic_atoms: SymbolicAtoms = _init.symbolic_atoms
        for atom in symbolic_atoms:
            symbol = atom.symbol
            plit = atom.literal
            slit = _init.solver_literal(plit)

            if plit > self.max_plit:
                self.max_plit = plit
            
            self.atomNames[symbol] = plit
            self.map_plit_slit[plit] = slit
            self.map_plit_slit[-plit] = -slit

            if slit not in self.map_slit_plit:
                self.map_slit_plit[slit] = []
            self.map_slit_plit[slit].append(plit)

            if -slit not in self.map_slit_plit:
                self.map_slit_plit[-slit] = []
            self.map_slit_plit[-slit].append(-plit)

        self.lits = [self.max_plit]

        facts = self.map_slit_plit.get(1, [])
        self.lits.extend(facts)

        propagator.atomNames = self.atomNames
        propagator.map_slit_plit = self.map_slit_plit
        propagator.map_plit_slit = self.map_plit_slit

    @staticmethod
    def cleanup():
        PropagatorClingoInitializer._instance = None
