# Tests

| Folder | What | How to run (from the repository root) |
|---|---|---|
| `unit/` | C++ unit tests of `amoclingo-core`: the `--progression` parser, `utility`, the grounding into `AmosumProgram` (groups, shared tuples, equal weights), the search algorithms, `AmosumClingoAssignment` | `make -C tests/unit test` |
| `correctness/` | end-to-end checks of `amoclingo-core` against plain clingo, on every benchmark and configuration | `python tests/correctness/check_correctness.py [--quick] [bench ...]` |
| `benchmarks/` | the ASP inputs of `correctness/`: `edge` (small cases), `ga`, `gc`, `kn`, `kn_toy` (`encoding-amosum-{amo,eo}.asp` + `instances/`), `kn_toy_max`, `kn_max` (`#amomaximize` encodings on the kn_toy / kn instances) | |

fiore's own tests are in `fiore/tests` (`make -C fiore test`).

Both need the `amosum` conda env (clingo; `CONDA_PREFIX` set) and `correctness/` needs `amoclingo-core` built
(`make -C amosum/amoclingo/cpp release`).

## Unit tests

- `unit.h`: a minimal framework without dependencies (`TEST`, `CHECK`, `CHECK_EQ`, `CHECK_THROWS`, `REQUIRE`).
- `test_main.cpp`: sets up what `amoclingo-core`'s main sets up (ArgParse, Statistics, logger) and runs the tests.
- `helpers.h`: a solver on `data/<name>.lp`, a search with a strategy and a progression, output capture.
- `data/*.asp`: the inputs in amosum syntax; `data/*.lp`: the same rewritten by the parser (`make -C tests/unit data`
  after changing an `.asp` or the parser), so the tests do not need Python.

`make -C tests/unit test` builds and runs a debug build (asserts and `LOG` on) and a release build (`-O3 -DNDEBUG`);
`make -C tests/unit run FILTER=search` runs only the tests whose name contains `search`. The exit code is 0 only if
every test passed.
