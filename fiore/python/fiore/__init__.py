"""fiore (Python): the parts of the C++ fiore library that a Python propagator needs.

- fiore.result      exit codes and the result of a search (pure Python)
- fiore.program     a clingo program: grounding, atoms by name and literal, extra ground rules
- fiore.solver      solve calls on a program, one model each
- fiore.propagator  a clingo propagator that dispatches to one engine per solver thread
- fiore.wasp        WASP: the plugin protocol (pure Python, runs inside WASP) and a runner for the binary
"""
