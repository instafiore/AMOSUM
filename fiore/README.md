# fiore

Reusable C++20 components (namespace `fiore`), meant to be copied into a project and compiled
from source together with it: modify the code in place, no install step, no prebuilt library.
Originally extracted from WeakSatBasedLS.

## Layout

fiore exists in C++ (`cpp/`) and in Python (`python/`, see below), mirroring each other: `cpp/fiore/<area>/<file>.h`
<-> `python/fiore/<area>/<file>.py`.

```
fiore/cpp/
├── fiore.mk          sources, include flag: include it from the application's Makefile
├── Makefile          fiore's own tests only (make -C fiore/cpp test)
├── tests/            test_utils (no Clingo), test_solver, data/*.asp
└── fiore/            the sources (included as "fiore/<area>/<file>.h")
    ├── utils/            generic, no Clingo                                      namespace fiore
    │   ├── constants.h       fiore::CONSTANTS (sentinels, exit codes, option keys)
    │   ├── utilityCommon.*   containers (Vector, IterableSet, Hash, Interpretation, ...), Literal, printers, timers
    │   ├── utility.*         ArgParse (command line), Cluster, WeightFunction, cleanup()
    │   ├── logger.*          Logger, LOG macro
    │   ├── stats.*           Statistics (counters, summaries, series, JSON)
    │   └── args-parser/      vendored header-only library (MIT, see its LICENSES/)
    ├── clingo/           everything that touches the Clingo C API                namespace fiore::clingo
    │   └── utilityClingo.*   ClingoResult, truth values of literals, errors (handleError), statistics, configuration
    ├── model/            Program and solver on top of Clingo                     namespace fiore
    │   ├── program.*         Program, PBConstraint, grounding
    │   ├── solver.*          ClingoSolver: solve, assumptions, unsat cores, polarity, createOptimizer
    │   ├── assigment.*       Assignment, ClingoAssignment, AnswersetsPool
    │   ├── propagator.*      ClingoPropagator (watches, clause type, addClause), ClingoPropagatorInitializer
    │   ├── propagatorPBConstraint.*, propagatorPBImp.*, propagatorPBImpLe.*   sum <= bound propagator
    │   ├── bound.*           Core, CoreLowerBound, LowerBound(OBV)
    │   └── propagatorGetRest.*, propagatorGetDerivedFromAss.*   helper propagators of ClingoSolver
```

`assigment` keeps its original spelling (all includes use it; the classes are spelled correctly).

## Using it in an application

```make
include path/to/fiore/cpp/fiore.mk
SRCS     += $(FIORE_SRCS)          # or only $(FIORE_UTILS_SRCS): no Clingo needed
CXXFLAGS += $(FIORE_CPPFLAGS)      # adds fiore/cpp to the include path
LDLIBS   += -lclingo               # for FIORE_CLINGO_SRCS
```
```cpp
#include "fiore/model/solver.h"
```
The sources are compiled with the application's own flags (C++20 required) into its own object
folder. Inside fiore all includes are relative, so the folder can be copied anywhere.

Initialisation, in this order (see `tests/test_solver.cpp`):

```cpp
fiore::startWallTime();
fiore::ArgParse::init(argc, argv, { /* the application's own fiore::Argument options */ });
fiore::Logger::init(file, lazy, level);         // debug builds only: LOG asserts that the logger exists
fiore::Statistics::initInstance(statsFile);
fiore::ClingoSolver solver({"encoding.asp", "instance.asp"});
...
fiore::cleanup();
```
`ArgParse` registers the options the library reads (`--log`, `--log-file-name`, `--TimestampLogFile`,
`--statsfile`, `--seed`, `--verbose`, `--polarity-heuristic`, `--dba`,
`--lb-progr-shuffle-select-point-strategy-mb`); the application passes its own options to `init`,
and they are parsed and validated together. `ArgParse` is optional for the generic utilities: an
application with its own command line (e.g. amosum) can use `Logger` and the containers without it.

Macros expand to qualified names: `LOG(...)` → `::fiore::Logger`, `handleError(...)` →
`::fiore::clingo::handle_error`. With `NDEBUG` undefined, `__debug__` is defined and enables `LOG`.
`-DNSTATS` compiles out the statistics collection.

## Tests

```bash
make test        # release and debug; STATS=0 to compile Statistics out
```
`test_utils` is compiled and linked without any Clingo include path or library.
`test_solver` checks unsat cores under assumptions and a linear SAT-UNSAT loop on the PB
propagator against `clingo --opt-mode=opt` (optima 1, 7, 89; satisfiable without optimisation;
unsatisfiable).

## Changes with respect to WeakSatBasedLS

- Namespace `fiore` (Clingo helpers in `fiore::clingo`); `std::hash<fiore::Mapping2Integer>`.
- `utils` is Clingo-free: `ClingoResult`, `EMPTY_LITERALS`, `clingoLogger` and `handleError` are in
  `clingo/utilityClingo`; `Statistics` no longer reads the solver (`ClingoSolver::updateStatistics`
  writes the Clingo counters into it).
- Removed (algorithm-specific): the algorithms, runner and main; options and constants of OBV, MB,
  SA, LSU and LB shuffling except the select-point strategy used by `LowerBoundOBV`; `Statistics`
  fields `lb`, `howManyAssumptionsUpToIndex`, the worsening-delta history; the `Decimal01OrClingo` regex.
- `ArgParse::init` accepts the application's options; default log and stats files are `fiore.log`
  and `fiore.stats`.
- No `using namespace std;` in headers (it was in `logger.h`).
- Bug fixes: `Set::contains` tested the element instead of its hash (pointer elements were never
  found, negative integers read out of bounds); `ClingoSolver::removeLastAssumption` did not update
  `assumptionsSet`.

## Python package (`python/`)

The parts of fiore that a Python propagator needs, as the package `fiore` (`python/fiore/`, installable with
`pip install -e fiore/python`), in the same layout as the C++ sources. Python has dicts, `argparse` and `logging`, so the
containers, `ArgParse`, `Logger` and `Statistics` have no Python counterpart; WASP exists only in Python.

```
python/fiore/
├── utils/constants.py                 ↔ utils/constants.h               exit codes, BOTTOM / TOP          pure Python
├── clingo/utility_clingo.py           ↔ clingo/utilityClingo.h          ClingoResult                      pure Python
├── model/program.py                   ↔ model/program.h                 Program: grounding, atoms, extra ground rules
├── model/solver.py                    ↔ model/solver.h                  ClingoSolver: one model per solve call
├── model/assignment.py                ↔ model/assigment.h               ClingoAssignment: a model as printed  pure Python
├── model/propagator.py                ↔ model/propagator.h              ClingoPropagator: clause type, add_clause
├── model/propagator_pb_imp.py         ↔ model/propagatorPBImp.h         PropagatorPBConstraintImplementation  pure Python
├── model/propagator_pb_constraint.py  ↔ model/propagatorPBConstraint.h  PropagatorPBConstraint: one implementation per
│                                                                         thread, volatile init conflicts through check
└── wasp/plugin.py, wasp/runner.py     (Python only)                      WASP's plugin protocol on the same
                                                                          implementations; the binary on a smodels program
```

A PropagatorPBConstraintImplementation works on solver literals and knows nothing about the solver:
`on_literals_true(changes, dl, force)` returns the derived literals (`BOTTOM` for a conflict),
`get_reason_for_literal(d)` the literals, false, that make `d` valid, `on_literals_undefined(changes)`,
`init_propagation()`. The same implementation runs on clingo (`PropagatorPBConstraint`) and inside WASP (`WaspPlugin`),
which is why the modules it needs are pure Python (WASP runs the plugin in its own interpreter, without clingo).
