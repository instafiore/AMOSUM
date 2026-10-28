# fiore

Reusable C++20 components (namespace `fiore`), meant to be copied into a project and compiled
from source together with it: modify the code in place, no install step, no prebuilt library.
Originally extracted from WeakSatBasedLS.

## Layout

```
fiore/
├── fiore.mk          sources, include flag: include it from the application's Makefile
├── Makefile          fiore's own tests only
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
│   ├── propagator.*      ClingoPropagator, ClingoPropagatorInitializer
│   ├── propagatorPBConstraint.*, propagatorPBImp.*, propagatorPBImpLe.*   sum <= bound propagator
│   ├── bound.*           Core, CoreLowerBound, LowerBound(OBV)
│   └── propagatorGetRest.*, propagatorGetDerivedFromAss.*   helper propagators of ClingoSolver
└── tests/            test_utils (no Clingo), test_solver, data/*.asp
```

`assigment` keeps its original spelling (all includes use it; the classes are spelled correctly).

## Using it in an application

```make
include path/to/fiore/fiore.mk
SRCS     += $(FIORE_SRCS)          # or only $(FIORE_UTILS_SRCS): no Clingo needed
CXXFLAGS += $(FIORE_CPPFLAGS)      # adds the parent folder of fiore/ to the include path
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
`pip install -e fiore/python`). Python has dicts, `argparse` and `logging`, so the containers, `ArgParse`, `Logger` and
`Statistics` have no Python counterpart.

```
python/fiore/
├── result.py        exit codes (10, 20, 30, 29, 28) and Result                         pure Python
├── program.py       Program: clingo grounding, atoms by name and literal, extra ground rules (add_rules)
├── solver.py        ClingoSolver: one model per solve call, with a snapshot of the model
├── propagator.py    EnginePropagator: a clingo propagator dispatching to one engine per solver thread; clauses
│                    learnt or volatile; init conflicts made volatile through check (as PropagatorPBConstraint)
└── wasp.py          WaspPlugin + install: WASP's plugin protocol on the same engines; run_wasp: runs the
                     binary on a ground program (smodels) and reads the models                pure Python
```

An engine works on solver literals and knows nothing about the solver: `on_true(changes, dl, force)` returns the
derived literals (`BOTTOM` for a conflict), `reason(d)` the literals, false, that make `d` valid, `undo(changes)`,
`init_check()`. The same engine runs on clingo (`EnginePropagator`) and inside WASP (`WaspPlugin`); `result.py` and
`wasp.py` are pure Python because WASP runs the plugin in its own interpreter.
