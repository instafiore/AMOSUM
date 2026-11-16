# JOINTSUM — design (v1)

Branch `jointsum`. Lecture: `lecture/jointsum-pipeline.pdf` (Part III has the details and the pictures).
Motivating example (option 3, plain clingo): `motivating_example/`.

## Decided

- Syntax: classical `#sum` syntax, no groups: `#jointsum{ W,T : cond ; ... } >= B : body.` and `<= B`.
- Only `#jointsum` statements take part. Each statement is a target, and the statements of the opposite
  direction that share literals with it are its side constraints.
- **Nested classes in v1**: the bound of a group is the LP of a multiple-choice knapsack (upper convex hull per class).
- Command `jointclingo`, clingo only. `amoclingo`, `amowasp` and `amosum/ijcai_version/` are untouched.

## Model (target `>= K`)

From the side constraints of a target:
- AMO side constraint (all weights 1, `<= 1`) → a **class**;
- any other `<= k` side constraint → a **capacity group** with capacity k;
- a class whose literals are all in a capacity group is **nested** in it; the other classes are groups with R = ∞;
- a literal in no side constraint → a one-option class with R = ∞ (UB = w);
- overlap that is not nesting: each literal goes to the side constraint that gives the best bound (weight splitting later);
- a side literal that is not in the target (weight 0) still uses capacity when true.

Per group: residual capacity `R = k − Σ cost of true literals`, and `T = Σ value of true options`.
Per class: the upper convex hull of `{(0,0)} ∪ {(c_i, p_i) : i undefined}` (cost = weight in the side constraint,
value = weight in the target), cut into segments (Δc, Δp, slope).
Group bound: `UB = T + ⌊Dantzig over all segments of all classes, capacity R⌋`, which is exact for the MCKP LP
because the slopes decrease inside each class. `mps = Σ UB`.

Dual form, used by the quick tests: `UB = T + λR + Σ_C g_C(λ)`, where λ is the slope of the critical segment and
`g_C(λ) = max(0, max_i (p_i − λ c_i))`.

Special cases:
- **Knapsack** (`in_knapsack(X,1..20)`): collinear options, so one segment per class, and the bound is the LP
  (0096: 1 273 508 < MIN 1 429 327).
- **amosum**: AMO only, R = ∞, so UB = the max undefined weight. Keep amosum's O(1) max: no hull is needed.
- **Flat item**: a one-option class.

## Data structures

- Per class: the options sorted by cost, and their hull, recomputed in O(k) when an option on the hull changes.
  When a dominated option changes, nothing is recomputed.
- Per capacity group: a treap of the current segments keyed by slope, with subtree sums (ΣΔc, ΣΔp).
  - Critical segment and UB: one descent, O(log n).
  - Class update: O(h log n), with h the hull size.
- Per thread: a copy, as for `AmosumThreadGroup`; the read-only data is shared, as for `AmosumSolvingGroups`.

## Propagation (slack s = mps − K)

- **¬l** if `UB − UB[l=⊤] > s`. The quick test is that the drop is ≥ `g_C(λ) − (p_l − λc_l)`. The exact test is to fix l
  (T += p_l, R −= c_l, remove C's segments), take one descent and restore.
- **l** if `UB − UB[l=⊥] > s`. Only hull vertices can qualify. The quick test is that the drop is ≥ `g_C(λ) − g_{C∖l}(λ)`.
  The exact test is to rebuild C's hull without l, take one descent and restore.
- Conflict if `mps < K`.

## Reasons

- Initial reason: the false options that shape the hulls, plus the true literals that use capacity or decide a class.
- Minimisation generalises `--reason min`. In a greedy pass, each candidate is restored (re-insert its point, update the
  hull and the treap) and `inc = UB_new − UB_old` is computed. The restore is kept if the cumulative inc ≤ s_h,
  otherwise undone. Everything is rolled back at the end. The recomputation is exact, so the reason is sound.

## Tests

1. Unit: the hull, the treap, and the MCKP LP against brute force on random small groups (bound ≥ integer optimum).
2. Oracle: `#jointsum` replaced by `#sum` in plain clingo gives the same answer sets / SAT-UNSAT.
3. amosum as jointsum (target plus one `<= 1` per group) behaves like `#amosum` (same choices and conflicts on `ga gc kn`).
4. Knapsack 0056/0096 with the natural encoding are UNSAT at the root.

## Answers (2026-11-16)

- Code may be shared with amosum, as long as amosum's behaviour and output stay identical.
- C++ only. The Python version is ignored, and Python code changes only where the commands need it to run
  (wrappers, parser).
- Optimisation (`#jointmaximize`): future work.
- Every statement, `>=` or `<=`, is both a target (it has its own propagator) and a side constraint of the others.
  The direction only decides how it enters a bound:
  - a `>=` target needs an upper bound: the `<=` statements cap it (capacity groups, AMO classes, upper hull), and the
    "at least one" of an EO class makes it pay the class's cheapest cost;
  - a `<=` target needs a lower bound, which is the mirror image (covering groups, lower hull).
  Both directions come in the same release. Statements also cooperate through propagation on assigned literals.
  A group restricted by `<=` and `>=` at once is a general LP with no greedy, so that is future work.
- Layout:
  - `jointsum/jointclingo/{__main__.py, cpp/}` (the command and its clingo glue);
  - `jointsum/core/cpp/jointsum/` (grounder reading, groups and classes, hulls, treap, propagator).
  - `amosum/` is unchanged (with `ijcai_version/`, `amoclingo`, `amowasp`, the Python version). Python changes only
    where the commands need it: the `jointclingo` wrapper, the `#jointsum` rule in the parser, `pyproject.toml`.
- **Nothing is implemented until the user says so.**
