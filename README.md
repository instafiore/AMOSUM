# AMOSUM

**AMOSUM** introduces a new class of constraints for **Answer Set Programming (ASP)** that tightly integrates the propagation power of **sum aggregates** and **at-most-one (AMO)** constraints.  

ASP already provides expressive modeling capabilities, but many real-world applications require stronger inference during solving. Aggregates such as `sum` and `count`, and exclusivity constraints such as `amo`, are heavily used, yet their independent propagation often leaves useful deterministic consequences undiscovered.

**AMOSUM addresses this gap.**

---

## Overview

- **AMOSUM constraints** combine additive information from `sum` aggregates with exclusivity information from `amo`, enabling propagation that is impossible when the constraints are treated independently.
- We introduce a **generalized notion of maximum possible sum**, extending inference beyond previously studied cases.
- We formalize new **sound propagation rules**, including propagation on false literals, and propose **efficient algorithms** for constructing **minimal reasons**, thereby improving clause learning and pruning the search space.
- The work provides both a complete **theoretical framework** and **practical solver integration**.

AMOSUM is implemented in both **WASP** and **Clingo**, and includes:

- Native language extensions:
  - `amosum` — at-most-sum constraints  
  - `eosum` — exactly-one-sum constraints
- Python-based and C-based propagators
- Lazy propagation options for improved runtime efficiency
- Extensive experimental evaluation showing **significant performance improvements**, including orders-of-magnitude speedups on selected benchmarks

---

## Selection Branch
- If you are not in branch ```amosum``` type:
    - git switch amosum
    
## Dependencies

- `clingo >= 5.8.0`  
- `g++`  
- Python 3.13

---

## Installation

Install requirements:

```bash

pip install -r requirements.txt
```

Update the submodules:

```bash
git submodule init 
git submodule update --remote --merge
```

Build the C-based Clingo propagator:

```bash
cd prop_clingo/propagator_clingo_c
make clean
make
cd ../../
```

---

## Running AMOSUM

### AMOCLINGO — C++ propagator

```bash
python ./prop_clingo/run.py -enc <encoding> -i <instance> -exp -lang c 
```

### AMOCLINGO — Python propagator

```bash
python ./prop_clingo/run.py -enc <encoding> -i <instance> -exp -lang py 
```

### AMOWASP — Python propagator

```bash
python ./prop_wasp/run.py -enc <encoding> -i <instance> -exp
```

---

## Lazy Propagation

Enable lazy propagation with:

```bash
-lang c -lazy
```

---

## Example Runs

### AMOCLINGO-INF-MR (Full propagation with minimization)

```bash
./prop_clingo/run.py \
    -enc tests/benchmarks/graph_colouring/encoding-amosum-amo.asp \
    -i tests/benchmarks/graph_colouring/instances/0001-graph_colouring-125-0_1200.asp \
    -exp -lang c
```

### AMOCLINGO-L (Lazy propagation)

```bash
./prop_clingo/run.py \
    -enc tests/benchmarks/graph_colouring/encoding-amosum-amo.asp \
    -i tests/benchmarks/graph_colouring/instances/0001-graph_colouring-125-0_1200.asp \
    -exp -lang c -lazy
```

