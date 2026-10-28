# AMOSUM — notes for Claude Code

AMOSUM extends ASP with `#amosum` / `#eosum` / `#amomaximize` constraints (sum aggregates aware of
at-most-one / exactly-one groups), with propagators for clingo (C++ and Python) and WASP (Python).
Paper: Alviano, Dodaro, Fiorentino, Maratea, "AMO-aware Aggregates in ASP", IJCAI 2024.

## How to work with the user

- **Show code in chat by default**; the user applies it and then asks to "check it". Edit files
  only when the user explicitly asks (e.g. "change the files", "integrate it", "can you do it").
- Don't run compilers or long experiments unless the task needs it or the user asks.
- Commit only when asked; **no references to Claude in commit messages** (no co-author line).
- When the user says "check it", review what they changed (`git diff`) and report concretely.

## Scope rules

- **Never modify `amosum/ijcai_version/`**: it is the published IJCAI version. The rest of the code
  is the extension being developed.
- **Never modify the Python versions** (Python propagators and all of `amosum/amowasp/`).
  The parser is a separate area; only touch it when the task is about it.

## Layout

- `amosum/` — Python package (`pyproject.toml`), C++ sources of `amoclingo-core` (on fiore):
  `amosum_program.*` (grounding into `AmosumProgram`/`AmosumConstraint`, groups),
  `amosum_propagator.*` (propagation per thread), `amosum_solver.*` (`AmosumClingoSolver`),
  `amosum_argparse.*`, `utility.{h,cpp}` (split, symbols, clingo errors), `settings.h`,
  `amoclingo/cpp/`: `amoclingo.cpp` (main), `runner_clingo.*`,
  `amoclingo_propagator.*` (clingo propagator), `amoclingo_algorithms.h` + `amoclingo_{sat_unsat,
  unsat_sat,binary,progression}.cpp` (searches for `#amomaximize`, `--progression`), Makefile;
  `amoclingo/python/` (Python propagator for clingo, `--lang py`), `amowasp/python/` (Python
  propagator for WASP), `amosum_parser/` (see below). The old C++ propagator (amosum.*,
  amosum_initializer.*, propagator_clingo*, optimizers) was removed: see git history before it.
- `fiore/` — reusable C++20 library (namespace `fiore`), compiled from source:
  - `utils/` (no Clingo): containers (`Vector`, `IterableSet`, `PerfectNHash`, `PerfectInterpretation`,
    `TruthValue`, ...), `ArgParse`, `Logger` + `LOG` macro, `Statistics`, `constants.h` (`fiore::CONSTANTS`)
  - `clingo/` (namespace `fiore::clingo`): Clingo helpers, `ClingoResult`, `handleError`
  - `model/`: `Program`, `ClingoSolver`, PB (sum <= bound) propagator, `Core`/`CoreLowerBound`, ...
  - `fiore.mk` (sources and include flag for applications), `Makefile` (`make test`), `README.md`
  - includes from outside: `#include "fiore/utils/logger.h"` with `-I<repo root>`
- `tests/` (see `tests/README.md`): `unit/` (C++ unit tests, own minimal framework `unit.h`:
  `make -C tests/unit test`), `correctness/check_correctness.py` (amoclingo-core against clingo),
  `benchmarks/` (`edge`, `ga`, `gc`, `kn`, `kn_toy`, `kn_toy_max`, `kn_max`).
- `experiments/` — paper material, not tests: `visualization/` (results, plots), `synthetic/`,
  `profiling/` (pprof scripts, still with the old amoclingo-core command line).

## C++ build and run

- Build (from `amosum/amoclingo/cpp/`): `make clean && make release`
  (what `install_amoclingo.sh` does); binary `bin/amoclingo-core`, expected there by `amoclingo`.
- The Makefile compiles the `.cpp` files of `amoclingo/cpp/` and of `amosum/` itself (not its
  subfolders) plus fiore's sources into `build/fiore/` (via `include ../../../fiore/fiore.mk`).
- **Debug build**: `make clean && make test` (`-g -O0`; AddressSanitizer is commented out in the
  Makefile: it aborts at startup on this Mac, even for an empty program).
- **Tests**: `make -C tests/unit test` (unit, debug and release), `python tests/correctness/check_correctness.py`
  (end to end, ~2 min; `--quick` for one configuration per case), `make -C fiore test` (fiore's own).
- Logging: `LOG(fiore::LogLevel::DEBUG|INFO|CRITICAL|ALWAYS, ...)`, active only without `NDEBUG`.
  Do not use `fiore::LogLevel::ERROR` for non-fatal messages: it exits the program.
  The log file is `--log-file-name` (default `fiore.log` in the current directory).
- Run: `/opt/homebrew/anaconda3/envs/amosum/bin/amoclingo <encoding> <instance> [--opt-strategy sat-unsat|unsat-sat|binary]
  [--progression 1|2|2^n|...] [--reason ...] [--lazy ...]` (conda env `amosum`; the base env has no `clingo`
  Python module). It rewrites the ASP files into temporary files and runs `amoclingo-core` with the options
  first (amoclingo-core's parser takes everything after the first file as a file). `-lg py` runs the
  Python propagator with the old `-e/-i` options. It is not maintained (left as it is on purpose): it supports
  only ge_amo, ge_eo, le_eo (le_amo and #amomaximize hit `assert False` in `get_propagator_variables`),
  its runner's `run()` crashes after solving since 9764811 (`results = None`, undefined `result`), and it
  counts equal tuples once per group (`#sum{T,G: C}`), while amoclingo-core uses set semantics
  (`#sum{T: C}`, the chosen semantics: equal tuples in different groups count once).
- **Benchmarks**: use the light ones (`ga`, `gc`, `kn`, `kn_toy`). **Do not use nurse rostering (`nr`)**: it is very heavy.
- Python: run from the repo root (`python -m amosum....`); from inside `amosum/`, the file
  `amosum.py` shadows the `amosum` package and imports fail.

## Recent history (2026-10)

- `fiore` library created from WeakSatBasedLS code; amosum's `common_utility`, `common_constants`,
  `logger`, `stats` removed and replaced by fiore (`fiore::PerfectNHash`, `fiore::setExitCode`,
  `fiore::Logger`). amosum keeps `SETTINGS::TRUE_STR/FALSE_STR` (`"true"/"false"`): fiore's are `"t"/"f"`.
- Groups are ordered deterministically (by weight, then literal id), as the old
  `amosum_initializer.cpp` did, so the order of the literals in a group never changes between runs.
- Light benchmarks were verified identical before/after each change (3 runs per instance).

## amosum_parser (current focus)

`amosum/amosum_parser/` (~750 lines Python, Lark LALR):
- `AmoSumGrammar.py` — ASP-Core grammar extended with `#amosum` / `#eosum` / `#amomaximize`
  elements `terms : naf_literals [group_id]`, bound `binop term`, optional `: body`.
- `AmoSumTransformer.py` — rewrites each amosum statement into plain ASP:
  `{__group__(lit, "+", W, (group_id), (aggregate_id, body))} :- lit, body.` per element,
  `__lb__(bound, aggregate_id)` / `__ub__(...)` (>= / <=), `__aux__(aggregate_id, "ge_amo"|"ge_eo"|"le_amo"|"le_eo")`,
  and an auxiliary `__group__(__aux__(...), "-", bound, ...)` choice. `#amomaximize` uses id
  `__amomaximizeid__`, bound 0, `>=`, prop type `amomaximize`.
  `aggregate_id` = (autoincrement, body without builtin atoms).
- `AspFactory.py` (tree builders), `ValidatorAmoSum.py`, `utils.py` (`toString` pretty printer,
  `debug`, `concatenate_nodes`), `__main__.py` (reads stdin, prints the rewritten program),
  `amosum.txt` (design notes), `test_cases/ge_amo_kn_amosum.asp`.
- Imports `from settings import *` (amosum's Python `settings.py`) and `from utility import *` via
  `sys.path` hacks in `__main__.py`. The parser had not been run yet in the session that wrote this file.
