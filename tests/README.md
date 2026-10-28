# Tests

| Folder | What | How to run (from the repository root) |
|---|---|---|
| `unit/cpp/` | C++ unit tests of `amoclingo-core`: the `--progression` parser, `utility`, the grounding into `AmosumProgram` (groups, shared tuples, equal weights), the search algorithms, `AmosumClingoAssignment` | `make -C tests/unit/cpp test` |
| `unit/python/` | Python unit tests of the Python version: `--progression`, the engine on hand-made literals (solving groups, propagation, reasons), the grounding and normalization, the searches on clingo and on WASP, fiore (Python) | `python -O -m unittest discover -s tests/unit/python -t .` |
| `unit/data/` | the inputs of both: `*.asp` in amosum syntax, `*.lp` the same rewritten by the parser (`make -C tests/unit/cpp data`) | |
| `correctness/` | end-to-end checks against plain clingo (set semantics), on every benchmark and configuration, for the three implementations | `python tests/correctness/check_correctness.py [--implementation cpp\|py\|wasp] [--quick] [bench ...]` |
| `benchmarks/` | the ASP inputs of `correctness/`: `edge` (small cases), `ga`, `gc`, `kn`, `kn_toy` (`encoding-amosum-{amo,eo}.asp` + `instances/`), `kn_toy_max`, `kn_max` (`#amomaximize` encodings on the kn_toy / kn instances) | |

fiore's own C++ tests are in `fiore/tests` (`make -C fiore test`).

Everything needs the `amosum` conda env (clingo; `CONDA_PREFIX` set for the C++ builds) with fiore's Python package
installed (`pip install -e fiore/python`). `--implementation cpp` needs `amoclingo-core` built
(`make -C amosum/amoclingo/cpp release`); `--implementation wasp` and the WASP unit tests need `WASP_EXE`.

## Unit tests

C++ (`unit/cpp/`):
- `unit.h`: a minimal framework without dependencies (`TEST`, `CHECK`, `CHECK_EQ`, `CHECK_THROWS`, `REQUIRE`).
- `test_main.cpp`: sets up what `amoclingo-core`'s main sets up (ArgParse, Statistics, logger) and runs the tests.
- `helpers.h`: a solver on `data/<name>.lp`, a search with a strategy and a progression, output capture.
- `make -C tests/unit/cpp test` builds and runs a debug build (asserts and `LOG` on) and a release build;
  `make -C tests/unit/cpp run FILTER=search` runs only the tests whose name contains `search`.

Python (`unit/python/`): `unittest` from the standard library; `helpers.py` grounds `data/<name>.lp` and runs a search
on clingo. The WASP tests are skipped without `WASP_EXE`.
