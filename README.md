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

- `make`
- `conda`

---

# Installation

```bash
conda env create -f environment.yml
bash ./install
```

## Additional build step for the IJCAI version

```bash
bash ./install_ijcai
```

---

# Usage

## Show help

```bash
amoclingo -h
```

---

# Running AMOSUM

## AMOCLINGO — C++ propagator

```bash
amoclingo -e <encoding> -i <instance> -l cpp
```

## AMOCLINGO — Python propagator

```bash
amoclingo -e <encoding> -i <instance> -l py
```

## AMOWASP — Python propagator

```bash
amowasp -e <encoding> -i <instance>
```

---

# Lazy Propagation

Lazy propagation can be enabled with:

```bash
--l=cpp --lazy=true
```

This strategy reduces propagation overhead by delaying reasoning until it becomes necessary during search.

---

# Example Runs

## Full propagation with IJCAI reason (AMOSUM-INF)

```bash
amoclingo \
    -e tests/benchmarks/graph_colouring/encoding-amosum-amo.asp \
    -i tests/benchmarks/graph_colouring/instances/0001-graph_colouring-125-0_1200.asp \
    -m ijcai
```

## Full propagation (AMOSUM-INF-R)

```bash
amoclingo \
    -e tests/benchmarks/graph_colouring/encoding-amosum-amo.asp \
    -i tests/benchmarks/graph_colouring/instances/0001-graph_colouring-125-0_1200.asp
```

## Full propagation with Minimization (AMOSUM-INF-MR)

```bash
amoclingo \
    -e tests/benchmarks/graph_colouring/encoding-amosum-amo.asp \
    -i tests/benchmarks/graph_colouring/instances/0001-graph_colouring-125-0_1200.asp \
    -m min
```

## Lazy propagation (AMOSUM-L)

```bash
amoclingo \
    -e tests/benchmarks/graph_colouring/encoding-amosum-amo.asp \
    -i tests/benchmarks/graph_colouring/instances/0001-graph_colouring-125-0_1200.asp \
    --lazy=true
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