# fiore: reusable C++ components, compiled from source together with the application.
#
# In the application's Makefile:
#
#     include path/to/fiore/fiore.mk
#     SRCS     += $(FIORE_SRCS)        # or only $(FIORE_UTILS_SRCS): the part that does not need Clingo
#     CXXFLAGS += $(FIORE_CPPFLAGS)    # then: #include "fiore/model/solver.h"
#     LDLIBS   += -lclingo             # needed by FIORE_CLINGO_SRCS
#
# The application keeps its own compiler flags (C++20 is required) and object directory.

FIORE_DIR := $(patsubst %/,%,$(dir $(lastword $(MAKEFILE_LIST))))

# generic utilities: containers, printers, ArgParse, Logger, Statistics; no Clingo
FIORE_UTILS_SRCS  := $(wildcard $(FIORE_DIR)/utils/*.cpp)
# Clingo helpers and the model: Program, ClingoSolver, propagators
FIORE_CLINGO_SRCS := $(wildcard $(FIORE_DIR)/clingo/*.cpp) $(wildcard $(FIORE_DIR)/model/*.cpp)
FIORE_SRCS        := $(FIORE_UTILS_SRCS) $(FIORE_CLINGO_SRCS)

# headers are included as "fiore/<area>/<file>.h"
FIORE_CPPFLAGS    := -I$(FIORE_DIR)/..
