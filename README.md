# AMOSUM

**AMOSUM** introduces a novel class of constraints for **Answer Set Programming (ASP)** that tightly integrates the propagation capabilities of **sum aggregates** and **at-most-one (AMO)** constraints.

ASP provides highly expressive modeling features, but many real-world applications require stronger inference during solving. Aggregates such as `sum` and `count`, together with exclusivity constraints like `amo`, are widely used in practical encodings. However, when these constraints are handled independently, propagation often misses deterministic consequences that emerge from their interaction.

**AMOSUM overcomes this limitation by unifying both forms of reasoning into a single constraint framework.**

---

# Overview

AMOSUM constraints exploit the synergy between:

- the additive reasoning of `sum` aggregates, and
- the exclusivity reasoning of `amo` constraints.

This integration enables significantly stronger propagation than treating the constraints separately.

This repository provides:

- A **novel constraint class** combining `sum` and `amo`
- A **generalized notion of maximum achievable sum**, extending propagation beyond previously studied approaches
- **Sound propagation rules**, including propagation on false literals
- Efficient algorithms for computing **minimal reasons**, improving clause learning and reducing the search space
- A complete **theoretical framework** together with **practical solver implementations**

---

# Features

AMOSUM includes:

- Native language extensions:
  - `amosum` — **at-most-sum constraints**
  - `eosum` — **exactly-one-sum constraints**
- Implementations for both **Clingo** and **WASP**
- **C++ propagators**
- **Python propagators**
- **Lazy propagation support**
- An **experimental evaluation framework**

Experimental results demonstrate **substantial performance improvements**, including **orders-of-magnitude speedups** on several benchmark domains.

---

# Dependencies

The following software is required:

* `make`
* `conda`
* [`wasp`](https://alviano.github.io/wasp/index.html), compiled with `SCRIPT=python38`

---

# Installation

Create the Conda environment, activate it and install everything at once:

```bash
conda env create -f environment.yml
conda activate amosum
./install.sh --wasp <path_to_wasp_exe>
```

`install.sh` installs the Python packages (`amosum`, with the commands `amoclingo`, `amowasp` and `amosumparser`, and
`fiore`), builds `amoclingo-core` (the C++ propagator) and stores the path of the `wasp` executable in the
environment (`WASP_EXE`, set at every `conda activate`), then checks the three commands on a small program.
`--wasp` can be left out if `WASP_EXE` is already set or `wasp` is on the `PATH`; without WASP, `amowasp` is not set
up. `./install.sh --no-pip` only rebuilds `amoclingo-core`; `./install.sh --help` lists the options.

## Additional build step for the IJCAI version

To build the IJCAI version, run:

```bash
source ./install_ijcai_version.sh [<path_to_wasp_exe>]
```

The `<path_to_wasp_exe>` argument is optional if the executable is already available through the environment configuration.


---

# Usage

```bash
amoclingo -h
amowasp -h
```

All the commands take the ASP files (encoding, instance, ...) as positional arguments and the same options.

## AMOCLINGO — C++ propagator (`amoclingo-core`)

```bash
amoclingo <encoding> <instance>
```

## AMOCLINGO — Python propagator (clingo's Python API)

```bash
amoclingo <encoding> <instance> -lg py
```

## AMOWASP — Python propagator on WASP

```bash
amowasp <encoding> <instance>          # WASP from $WASP_EXE, or --wasp <path>
```

The Python version is a port of the C++ one: same semantics, options, output and exit codes (10 SAT, 20 UNSAT,
30 optimum). An equal tuple in two groups counts once (as clingo's `#sum{T: C}`).

## Options

| Option | Values | Default |
|---|---|---|
| `--reason` | `nomin`, `ijcai` (all the false literals of the group), `min`, `minfly` (minimized reasons), `cmin` | `nomin` |
| `--lazy` | `false`, `true`, `hybrid`, or a float in [0,1]: when to propagate | `false` |
| `--opt-strategy` | `#amomaximize`: `sat-unsat` (upwards), `unsat-sat` (downwards), `binary` | `sat-unsat` |
| `--progression` | step of the bound at step n for `sat-unsat` / `unsat-sat`: `1`, `2`, `2^n`, `n^2`, ... | `1` |

On WASP each step of an `#amomaximize` search is a new WASP run.

---

# Example Runs

## Full propagation with IJCAI reason (AMOSUM-INF)

```bash
amoclingo tests/benchmarks/gc/encoding-amosum-amo.asp tests/benchmarks/gc/instances/0001-graph_colouring-125-0_1200.asp \
    --reason ijcai
```

## Full propagation (AMOSUM-INF-R)

```bash
amoclingo tests/benchmarks/gc/encoding-amosum-amo.asp tests/benchmarks/gc/instances/0001-graph_colouring-125-0_1200.asp
```

## Full propagation with Minimization (AMOSUM-INF-MR)

```bash
amoclingo tests/benchmarks/gc/encoding-amosum-amo.asp tests/benchmarks/gc/instances/0001-graph_colouring-125-0_1200.asp \
    --reason min
```

## Lazy propagation (AMOSUM-L)

```bash
amoclingo tests/benchmarks/gc/encoding-amosum-amo.asp tests/benchmarks/gc/instances/0001-graph_colouring-125-0_1200.asp \
    --lazy true
```

## #amomaximize with exponential steps

```bash
amoclingo tests/benchmarks/kn_toy_max/encoding-amomaximize.asp tests/benchmarks/kn_toy/instances/toy_le_5.asp \
    --opt-strategy unsat-sat --progression "2^n"
```

---

# IJCAI Version

## AMOWASP-BASE-PY

```bash
bash amosum/ijcai_version/AMOSUM/amowasp-base.sh <encoding> <instance>
```

## AMOCLINGO-BASE-PY

```bash
bash amosum/ijcai_version/AMOSUM/amoclingo-base-py.sh <encoding> <instance>
```

## AMOCLINGO-BASE-C

```bash
bash amosum/ijcai_version/AMOSUM/amoclingo-base-c.sh <encoding> <instance>
```

---

# Publication

If you use AMOSUM in your research, please cite:

```bibtex
@inproceedings{DBLP:conf/ijcai/AlvianoDFM24,
  author       = {Mario Alviano and
                  Carmine Dodaro and
                  Salvatore Fiorentino and
                  Marco Maratea},
  title        = {AMO-aware Aggregates in Answer Set Programming},
  booktitle    = {Proceedings of the Thirty-Third International Joint Conference on
                  Artificial Intelligence, {IJCAI} 2024, Jeju, South Korea, August 3-9,
                  2024},
  pages        = {3215--3223},
  publisher    = {ijcai.org},
  year         = {2024},
  url          = {https://www.ijcai.org/proceedings/2024/356}
}
```
