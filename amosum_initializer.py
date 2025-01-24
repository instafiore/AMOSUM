import re
import json
from collections import defaultdict
import json
import sys
import os


sys.path.append(os.path.dirname(os.path.abspath(__file__)))
from ast import Tuple
from typing import Callable, List
from utility import *
import prop_wasp.propagator_wasp_py.wasp as wasp
import re
import settings
from settings import *
import time
import traceback

# Assuming the related functions and classes like WeightFunction, GroupFunction, AggregateFunction, etc., are defined elsewhere in the project.
class GenericData:
    def __init__(self):
        self.bound = None  # Default equivalent to SETTINGS::NONE
        self.groups_raw = {}  # A dictionary with string keys and list of literals as values
        self.bind = []  # A list of literals
        self.weights_names = {}  # A dictionary with string keys and integer values
    
    def __repr__(self):
        return f"GenericData(bound={self.bound}, groups_raw={self.groups_raw}, bind={self.bind}, weights_names={self.weights_names})"


class AmoSumInitializer:
    DEFAULT_LAZY = settings.FALSE
    instance = None
    WASP = 1
    CLINGO = 2

    def __init__(self):
        self.first = True
        self.weight = None
        self.aggregate_map = {}
        self.generic_data_map = {}
        self.strategy = None
        self.ID = "0"
        self.groups = []
        self.lazy_prop_activated = False
        self.lazy_condition = False
        self.groups_literals = []

    @staticmethod
    def get_instance():
        if AmoSumInitializer.instance is None:
            AmoSumInitializer.instance = AmoSumInitializer()
        return AmoSumInitializer.instance

    def getLiterals(self, lits, amosum_propagator):
        amosum_propagator.N = lits[0] + 1
        amosum_propagator.minimization = amosum_propagator.param.get("min_r",Minimize.NO_MINIMIZATION.value)
        amosum_propagator.strategy = amosum_propagator.param.get("strategy",self.strategy)
        amosum_propagator.I = SymmetricFunction(amosum_propagator.N)
        amosum_propagator.group = GroupFunction(amosum_propagator.N)
        amosum_propagator.propagated = SymmetricFunction(amosum_propagator.N)
        amosum_propagator.aggregate = AggregateFunction(amosum_propagator.N, False)
        amosum_propagator.to_be_propagated = PerfectHash(amosum_propagator.N, False)
        amosum_propagator.reason = PerfectHash(amosum_propagator.N, [])
        amosum_propagator.redundant_lits = PerfectHash(amosum_propagator.N, [])
        amosum_propagator._mps = 0
        amosum_propagator.ID = amosum_propagator.param.get("id","0")
        amosum_propagator.groups = []
        amosum_propagator.assumptions = amosum_propagator.param.get("ass", False)
        amosum_propagator.current_sum = 0
        amosum_propagator.groups_literals = []

        if self.first:
            self.weight = WeightFunction(amosum_propagator.N)
            self.first = False
            self.common_phase(amosum_propagator)

        self.assign(amosum_propagator)
        self.specific_phase(lits, amosum_propagator)

        gd = self.generic_data_map[amosum_propagator.ID]
        return gd.bind

    def common_phase(self, amosum_propagator):
        # Assuming a debug function and utility functions like `create_atomNames_string` and `from_symbol_to_string` are defined.
        set_debug(amosum_propagator.param.get("d",""))
        for symbolic_atom, literal in amosum_propagator.atomNames.items():
            a = symbolic_atom
            if a.startswith(PREDICATE_GROUP + "("):
                # Process PREDICATE_GROUP type
                terms = wasp.getTerms(PREDICATE_GROUP, a)
                if len(terms) != 5 :
                    continue
                id_str = terms[4] 
                group_literal = literal
                amosum_propagator.groups_literals.append(not_(group_literal))
                lit_str = terms[0]
                atom_name = lit_str
                match = re.match(r"^not\s+(?P<atom_name>[\w()]+)", lit_str)
                if match:
                    atom_name = match.group("atom_name")
                    lit = amosum_propagator.atomNames[atom_name] * -1
                else:
                    lit = amosum_propagator.atomNames[atom_name]

                sign = 1 if "+" in terms[1] else -1
                lit *= sign

                weight = int(terms[2])
                self.weight[lit] = weight

                group_id = terms[3]
                gd = self.generic_data_map.setdefault(id_str, GenericData())
                gd.weights_names[atom_name] = weight

                # Update groups and aggregates
                gd.groups_raw.setdefault(group_id, [])
                gd.groups_raw[group_id].append(lit)
                if id_str not in self.aggregate_map:
                    self.aggregate_map[id_str] = AggregateFunction(amosum_propagator.N)
                    
                self.aggregate_map[id_str][lit] =  True

                gd.bind.append(lit)
                gd.bind.append(-lit)
                self.generic_data_map[group_id] = gd

            elif a.startswith(PREDICATE_LB + "(") or a.startswith(PREDICATE_UB + "("):
                # Process bounds
                bound_str = PREDICATE_LB if a.startswith(PREDICATE_LB + "(") else PREDICATE_UB 
                terms = wasp.getTerms(bound_str, a)
                if len(terms) != 2:
                    continue
                id_str = terms[1]
                gd = self.generic_data_map.setdefault(id_str, GenericData())
                if gd.bound is not None:
                    raise ValueError("Bound is already set!")
                gd.bound = int(terms[0])

    def assign(self, amosum_propagator):
        ID = amosum_propagator.ID
        amosum_propagator.aggregate = self.aggregate_map[ID]
        amosum_propagator.weight = self.weight
        if amosum_propagator.ge: amosum_propagator.lb = self.generic_data_map[ID].bound
        else: amosum_propagator.ub = self.generic_data_map[ID].bound
        amosum_propagator.bound = self.generic_data_map[ID].bound
        amosum_propagator.weights_names = self.generic_data_map[ID].weights_names

    def specific_phase(self, lits, amosum_propagator):
        max_diff = 0
        ID = amosum_propagator.ID
        lazy_param = str(amosum_propagator.param.get("lazy", self.DEFAULT_LAZY))
        amosum_propagator.lazy_prop_activated = not re.search(lazy_param,settings.FALSE, re.IGNORECASE)
        amosum_propagator.lazy_condition = not amosum_propagator.lazy_prop_activated
        lazy_hybrid = re.search(lazy_param,settings.LAZY_HYBRID, re.IGNORECASE)

        check_mps = amosum_propagator.param.get("check_mps", False)
        debug(f"id: {amosum_propagator.ID} total_weight_names: {json.dumps(amosum_propagator.weights_names)}", force_print=True) if amosum_propagator.solver == AmoSumInitializer.WASP and check_mps else None

        # Logging and preprocessing
        for group_id, lits_group in self.generic_data_map[ID].groups_raw.items():
            lits_ord = sorted([(lit, self.weight[lit]) for lit in lits_group], key=lambda x: x[1])
            ord_l = [lit for lit, _ in lits_ord]
            ord_i = {lit: i for i, (lit, _) in enumerate(lits_ord)}

            G = Group(ord_l, ord_i, group_id)
            ml = m_w(G, amosum_propagator.ge)

            max_w = self.weight[ml]
            min_w = self.weight[m_w(G, not amosum_propagator.ge)] if amosum_propagator.lazy_prop_activated or not amosum_propagator.ge else 0

            amosum_propagator._mps += max_w
            diff = abs(max_w - min_w)
            is_aux = re.search(settings.REGEX_AUX, get_name(atomNames=amosum_propagator.atomNames, lit = ml))
            if max_diff < diff and not is_aux:
                max_diff = diff

            amosum_propagator.groups.append(G)

            for lit in lits_group:  amosum_propagator.group[lit] = G

        nGroup = Group.autoincrement
        amosum_propagator.true_group = TrueGroupFunction(nGroup)

        debug(f"max_diff: {max_diff} lazy_prop_activated: {amosum_propagator.lazy_prop_activated}", force_print=True)

        amosum_propagator.lazy_perc = float(lazy_param) if amosum_propagator.lazy_prop_activated and re.search(lazy_param,settings.TRUE, re.IGNORECASE) and not lazy_hybrid else None
        if re.search(lazy_param,settings.TRUE, re.IGNORECASE) : amosum_propagator.lazy_perc = 1
        elif lazy_hybrid or re.search(lazy_param,settings.FALSE, re.IGNORECASE):  amosum_propagator.lazy_perc = amosum_propagator.lb / (amosum_propagator.lb + max_diff) if amosum_propagator.ge else (amosum_propagator.ub - max_diff) / amosum_propagator.ub
        assert not amosum_propagator.lazy_perc is None
        # Debugging lazy threshold and propagation
        lazy_str = f" lazy threshold {amosum_propagator.lazy_perc}" if amosum_propagator.lazy_prop_activated else ""
        debug(f"Starting propagator with param {amosum_propagator.param}{lazy_str}", force_print=True)

        for i in range(1, len(lits)):
            l = lits[i]
            try:
                amosum_propagator.update_phase(l)
                amosum_propagator.inconsistent_at_level_0 = False
            except Exception as e:
                amosum_propagator.inconsistent_at_level_0 = True
                raise e

        amosum_propagator.facts = lits[1:]
        amosum_propagator.last_decision_lit = 1
        amosum_propagator.dl = 0


    @staticmethod
    def cleanup():
        if AmoSumInitializer.instance is not None:
            AmoSumInitializer.instance = None
