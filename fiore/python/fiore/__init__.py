"""fiore (Python): the parts of the C++ fiore library that a Python propagator needs, in the same layout
(fiore/cpp/fiore/<area>/<file>.h <-> fiore/python/fiore/<area>/<file>.py):

- utils/constants             exit codes, BOTTOM / TOP
- clingo/utility_clingo       ClingoResult
- model/program               Program: clingo grounding, atoms by name and literal, extra ground rules
- model/solver                ClingoSolver: one model per solve call
- model/assignment            ClingoAssignment: a model as printed
- model/propagator            ClingoPropagator: clause type, add_clause
- model/propagator_pb_imp     PropagatorPBConstraintImplementation: what runs in each solver thread
- model/propagator_pb_constraint  PropagatorPBConstraint: one implementation per thread, volatile init conflicts
- wasp/plugin, wasp/runner    WASP (Python only): the plugin protocol on the same implementations, the binary

utils, clingo/utility_clingo, model/assignment, model/propagator_pb_imp and wasp are pure Python (the WASP plugin runs
in WASP's interpreter, without clingo).
"""
