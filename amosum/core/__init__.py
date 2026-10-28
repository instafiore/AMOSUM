"""amosum in Python (pure Python, no clingo: the WASP plugin imports it inside WASP's interpreter).

- progression  --progression: the step of the bound at step n (port of the C++ Progression)
- engine       the propagation of one aggregate on solver literals (port of AmosumPropagator and of the solving
               groups of AmoclingoPropagator)
- search       sat-unsat, unsat-sat, binary on the #amomaximize value (port of amoclingo_{sat_unsat,unsat_sat,binary})
- output       the printed lines and exit codes of amoclingo-core

The grounding side (reading the aggregates from the ground program, normalization) needs clingo: amosum.core.program.
"""
