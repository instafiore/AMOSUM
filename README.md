# AMOSUM

**AMOSUM** introduces a novel class of constraints for **Answer Set Programming (ASP)** that tightly integrates the propagation power of **sum aggregates** and **at-most-one (AMO)** constraints.

ASP offers highly expressive modeling capabilities, but many real-world applications require stronger inference during solving. Aggregates such as `sum` and `count`, and exclusivity constraints like `amo`, are widely used in practice. However, when handled independently, their propagation often fails to uncover deterministic consequences that arise from their interaction.

**AMOSUM addresses this limitation by combining both forms of reasoning into a unified constraint.**

---

# Overview

AMOSUM constraints exploit the interaction between additive information from `sum` aggregates and exclusivity information from `amo`, enabling stronger propagation than what is possible when the constraints are considered separately.

This repository provides:

- A **new constraint class** combining `sum` and `amo`
- A **generalized notion of maximum possible sum**, extending inference beyond previously studied cases
- **Sound propagation rules**, including propagation on false literals
- Efficient algorithms for computing **minimal reasons**, improving clause learning and reducing search space
- A complete **theoretical framework** together with **practical solver implementations**

---

# Implementations

AMOSUM is implemented for both **Clingo** and **WASP**.

The repository includes:

- Native language extensions:
  - `amosum` — **at-most-sum constraints**
  - `eosum` — **exactly-one-sum constraints**
- **C++ propagators**
- **Python propagators**
- **Lazy propagation support**
- An **experimental evaluation framework**

Experiments show **significant performance improvements**, including **orders-of-magnitude speedups** on several benchmark problems.

---

# Dependencies

The following software is required:

- `clingo >= 5.8.0`
- `g++`
- `Python >= 3.13`

---

# Installation


```bash
bash ./install
pip install .
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

Lazy propagation can be enabled using:

```bash
--l=cpp --lazy=true
```

This reduces propagation overhead by delaying reasoning until it becomes necessary during search.

---

# Example Runs

## Full propagation (AMOSUM-INF-MR)

```bash
amoclingo \
    -e tests/benchmarks/graph_colouring/encoding-amosum-amo.asp \
    -i tests/benchmarks/graph_colouring/instances/0001-graph_colouring-125-0_1200.asp
```

---

## Lazy propagation (AMOSUM-L)

```bash
amoclingo \
    -e tests/benchmarks/graph_colouring/encoding-amosum-amo.asp \
    -i tests/benchmarks/graph_colouring/instances/0001-graph_colouring-125-0_1200.asp \
    --lazy=true
```