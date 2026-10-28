"""
Correctness checks of amoclingo-core (the C++ propagator) against plain clingo.

Each case is an encoding with #amosum / #eosum / #amomaximize plus an instance. The reference is the same program with
every aggregate written as a plain clingo #sum (set semantics, as amosum: equal tuples in different groups count once)
and #amomaximize as #maximize. For each case and each configuration (--reason, --lazy) it checks:
  - the exit code: 20 if the reference is UNSAT, 10 if SAT, 30 with #amomaximize;
  - the printed answer set is an answer set of the reference (all its atoms true, every other atom false);
  - #amomaximize: the printed values strictly increase, the last one is the value of the printed answer set
    and the reference optimum.
The #amomaximize cases run with every --opt-strategy (sat-unsat, unsat-sat, binary) and --progression (1, 2, 2^n, n^2;
binary does not use it).
The Python wrapper (python -m amosum.amoclingo) is also checked on a few cases, with the options after the files.

Run from the repository root, with the amosum conda env (it needs the clingo Python module) and amoclingo-core built:
    python tests/correctness/check_correctness.py                  # every benchmark
    python tests/correctness/check_correctness.py edge kn_toy      # some of them
    python tests/correctness/check_correctness.py --quick          # one configuration per case
Exit code 0 when every check passes, 1 otherwise.
"""
import argparse
import glob
import os
import re
import subprocess
import sys
import tempfile
from concurrent.futures import ThreadPoolExecutor
from dataclasses import dataclass, field
from typing import Dict, List, Optional, Tuple

import clingo

REPO = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
HERE = os.path.join(REPO, "tests", "correctness", "bench")
OLD = os.path.join(REPO, "tests", "correctness-old", "test_suite", "bench")
EXE = os.path.join(REPO, "amosum", "amoclingo", "propagator_clingo_c", "bin", "amoclingo-core")

SAT, UNSAT, OPTIMUM = 10, 20, 30

REASONS = ["nomin", "ijcai", "min", "minfly"]
LAZY = ["false", "true", "hybrid", "0.5"]
ALL_CONFIGS = [(r, l) for r in REASONS for l in LAZY]
FEW_CONFIGS = [("nomin", "false"), ("min", "hybrid"), ("ijcai", "true")]
# --opt-strategy x --progression: every pair runs on the #amomaximize cases; without #amomaximize each one is a single
# solve call, checked once per pair on the edge cases only
STRATEGIES = ["sat-unsat", "unsat-sat", "binary"]
PROGRESSIONS = ["1", "2", "2^n", "n^2"]


# ---------------------------------------------------------------------------------------------------------------------
# benchmarks: name -> list of (encoding, instance or None), and the configurations to run
# (nurse rostering, nr, is left out on purpose: it is too heavy)

def benchmarks() -> Dict[str, Tuple[List[Tuple[str, Optional[str]]], List[Tuple[str, str]]]]:
    def old(bench: str, kinds=("amo", "eo")):
        instances = sorted(glob.glob(os.path.join(OLD, bench, "instances", "*.asp")))
        return [(os.path.join(OLD, bench, f"encoding-amosum-{k}.asp"), i) for k in kinds for i in instances]

    kn_toy_instances = sorted(glob.glob(os.path.join(OLD, "kn_toy", "instances", "*.asp")))
    return {
        "edge": ([(f, None) for f in sorted(glob.glob(os.path.join(HERE, "edge", "*.asp")))], ALL_CONFIGS),
        "kn_toy": (old("kn_toy"), ALL_CONFIGS),
        "kn_toy_max": ([(os.path.join(HERE, "kn_toy_max", "encoding-amomaximize.asp"), i) for i in kn_toy_instances], ALL_CONFIGS),
        "kn_max": ([(os.path.join(HERE, "kn_max", "encoding-amomaximize.asp"), i) for i in sorted(glob.glob(os.path.join(OLD, "kn", "instances", "*.asp")))], FEW_CONFIGS),
        "kn": (old("kn"), ALL_CONFIGS),
        "gc": (old("gc"), FEW_CONFIGS),
        "ga": (old("ga"), FEW_CONFIGS),
    }


# ---------------------------------------------------------------------------------------------------------------------
# reference: plain clingo

STATEMENT = re.compile(r"#(amosum|eosum|amomaximize)\s*\{")
BOUND = re.compile(r"\s*(<=|>=)\s*([^:.]+?)\s*(?::(.*?))?\.(?!\.)", re.S)     # >= B [: body].  ('..' is a range)
END = re.compile(r"\s*\.(?!\.)")


def splitTopLevel(text: str, separator: str) -> List[str]:
    # text split on separator outside parentheses and brackets
    parts, depth, current = [], 0, ""
    for ch in text:
        depth += ch in "([{"
        depth -= ch in ")]}"
        if ch == separator and depth == 0: parts.append(current); current = ""
        else: current += ch
    return parts + [current]


def plain(text: str) -> str:
    # #amosum{T1: C1 [G1]; T2: C2 [G2]} >= B : body.  ->  :- #sum{T1: C1; T2: C2} < B, body.
    # #eosum the same (exactly-one is a promise of the program), #amomaximize{...}.  ->  #maximize{...}.
    text = re.sub(r"%.*", "", text)
    out, pos = "", 0
    while True:
        m = STATEMENT.search(text, pos)
        if m is None: return out + text[pos:]
        out += text[pos:m.start()]
        i, depth = m.end(), 1                       # matching }
        while depth:
            depth += text[i] == "{"
            depth -= text[i] == "}"
            i += 1
        elements = [re.sub(r"\[[^\[\]]*\]\s*$", "", e.strip()).strip() for e in splitTopLevel(text[m.end():i - 1], ";")]
        aggregate = "#sum{%s}" % "; ".join(e for e in elements if e)
        if m.group(1) == "amomaximize":
            end = END.match(text, i)
            out += "#maximize{%s}." % aggregate[5:-1]
        else:
            end = BOUND.match(text, i)
            op, bound, body = end.groups()
            out += ":- %s %s %s%s." % (aggregate, "<" if op == ">=" else ">", bound.strip(), (", " + body.strip()) if body else "")
        pos = end.end()


def ground(text: str, instance: Optional[str], args: List[str] = ()) -> clingo.Control:
    ctl = clingo.Control(list(args) + ["--warn=none"])
    ctl.add("base", [], text)
    if instance: ctl.load(instance)
    ctl.ground([("base", [])])
    return ctl


@dataclass
class Reference:
    sat: bool
    optimum: Optional[int]          # #amomaximize value of an optimal answer set (None without #amomaximize)


def reference(text: str, instance: Optional[str], maximize: bool) -> Reference:
    ctl = ground(text, instance, ["--opt-mode=opt"])
    costs = []
    result = ctl.solve(on_model=lambda m: costs.append(list(m.cost)))
    if not result.satisfiable: return Reference(False, None)
    return Reference(True, -costs[-1][0] if maximize and costs and costs[-1] else (0 if maximize else None))


def checkAnswerSet(text: str, instance: Optional[str], atoms: List[str], maximize: bool) -> Tuple[bool, Optional[int]]:
    # is atoms exactly an answer set of the reference (as assumptions: atoms true, every other atom false)? its value
    ctl = ground(text, instance)
    true = set(clingo.parse_term(a) for a in atoms)
    assumptions = [(sa.symbol, sa.symbol in true) for sa in ctl.symbolic_atoms]
    known = set(sa.symbol for sa in ctl.symbolic_atoms)
    if not true <= known: return False, None
    costs = []
    result = ctl.solve(assumptions=assumptions, on_model=lambda m: costs.append(list(m.cost)))
    if not result.satisfiable: return False, None
    return True, (-costs[-1][0] if maximize and costs and costs[-1] else (0 if maximize else None))


# ---------------------------------------------------------------------------------------------------------------------
# amoclingo-core

def rewrite(path: str, directory: str) -> str:
    # the amosum statements rewritten into plain ASP by the parser (the same rewriting as the Python wrapper)
    with open(path) as f: text = f.read()
    out = subprocess.run([sys.executable, "-O", "-m", "amosum.amosum_parser"], input=text, capture_output=True, text=True, cwd=REPO)
    if out.returncode != 0: raise RuntimeError(f"the parser failed on {path}:\n{out.stderr[-2000:]}")
    rewritten = os.path.join(directory, re.sub(r"\W", "_", os.path.relpath(path, REPO)) + ".lp")
    with open(rewritten, "w") as f: f.write(out.stdout)
    return rewritten


@dataclass
class Run:
    exitCode: int
    atoms: Optional[List[str]]      # the last printed answer set
    values: List[int]               # "Amomaximize Cost: V" of each model found
    output: str


ANSWER_SET = re.compile(r"AnswerSet \d+° : \{(.*)\}\s*$")
VALUE = re.compile(r"^Amomaximize Cost: (-?\d+) ")


def parseOutput(exitCode: int, output: str) -> Run:
    atoms, values = None, []
    for line in output.splitlines():
        m = VALUE.match(line)
        if m: values.append(int(m.group(1)))
        m = ANSWER_SET.search(line)
        if m: atoms = re.findall(r"'([^']*)'", m.group(1))
    return Run(exitCode, atoms, values, output)


def runCore(inputs: List[str], strategy: str, progression: str, reason: str, lazy: str, directory: str, timeout: int) -> Run:
    # the options before the files: amoclingo-core takes everything after the first file as a file
    command = [EXE, f"--statsfile={os.path.join(directory, 'stats')}", f"--opt-strategy={strategy}",
               f"--progression={progression}", f"--reason={reason}", f"--lazy={lazy}", *inputs]
    try:
        out = subprocess.run(command, capture_output=True, text=True, timeout=timeout, cwd=directory)
    except subprocess.TimeoutExpired:
        return Run(-1, None, [], "TIMEOUT")
    return parseOutput(out.returncode, out.stdout + out.stderr)


def runWrapper(inputs: List[str], reason: str, lazy: str, directory: str, timeout: int) -> Run:
    # as a user would: python -m amosum.amoclingo FILES --reason R --lazy L
    command = [sys.executable, "-O", "-m", "amosum.amoclingo", *inputs, "--reason", reason, "--lazy", lazy,
               "--statsfile", os.path.join(directory, "stats")]
    try:
        out = subprocess.run(command, capture_output=True, text=True, timeout=timeout, cwd=REPO)
    except subprocess.TimeoutExpired:
        return Run(-1, None, [], "TIMEOUT")
    return parseOutput(out.returncode, out.stdout + out.stderr)


# ---------------------------------------------------------------------------------------------------------------------
# checks

@dataclass
class Case:
    bench: str
    encoding: str
    instance: Optional[str]
    plainText: str = ""
    maximize: bool = False
    ref: Optional[Reference] = None
    rewritten: List[str] = field(default_factory=list)

    @property
    def name(self) -> str:
        enc = os.path.basename(self.encoding)
        return f"{self.bench}/{enc}" + (f" {os.path.basename(self.instance)}" if self.instance else "")


def check(case: Case, run: Run) -> List[str]:
    # the problems of a run (empty: correct)
    errors = []
    ref = case.ref
    if run.exitCode == -1: return ["timeout"]
    if "could not be opened" in run.output or "error" in run.output.lower() and "Clingo" in run.output:
        errors.append("clingo error: " + " | ".join(l for l in run.output.splitlines() if "rror" in l)[:300])
    expected = UNSAT if not ref.sat else (OPTIMUM if case.maximize else SAT)
    if run.exitCode != expected:
        errors.append(f"exit code {run.exitCode}, expected {expected}")
    if not ref.sat:
        if run.atoms is not None: errors.append("an answer set was printed for an UNSAT program")
        return errors
    if run.atoms is None:
        return errors + ["no answer set printed"]
    isAnswerSet, value = checkAnswerSet(case.plainText, case.instance, run.atoms, case.maximize)
    if not isAnswerSet:
        errors.append(f"the printed answer set is not an answer set: {run.atoms[:20]}")
    if case.maximize:
        if not run.values:
            errors.append("no 'Amomaximize Cost' printed")
        else:
            if any(a >= b for a, b in zip(run.values, run.values[1:])):
                errors.append(f"values not strictly increasing: {run.values}")
            if run.values[-1] != ref.optimum:
                errors.append(f"last value {run.values[-1]}, optimum {ref.optimum}")
        if isAnswerSet and value != ref.optimum:
            errors.append(f"the printed answer set has value {value}, optimum {ref.optimum}")
    return errors


def main() -> int:
    all = benchmarks()
    parser = argparse.ArgumentParser(description="Correctness checks of amoclingo-core against clingo")
    parser.add_argument("benchmarks", nargs="*", metavar="bench", help=f"benchmarks to run ({', '.join(all)}; default: all)")
    parser.add_argument("--quick", action="store_true", help="one configuration (nomin, false) per case")
    parser.add_argument("--timeout", type=int, default=120, help="seconds per run [120]")
    parser.add_argument("--jobs", type=int, default=os.cpu_count(), help="parallel runs [cpu count]")
    parser.add_argument("--no-wrapper", action="store_true", help="skip the checks of the Python wrapper")
    parser.add_argument("--strategies", nargs="+", choices=STRATEGIES, default=STRATEGIES,
                        help="--opt-strategy values to check [all]")
    parser.add_argument("--progressions", nargs="+", default=PROGRESSIONS,
                        help=f"--progression values to check [{' '.join(PROGRESSIONS)}]")
    args = parser.parse_args()
    args.benchmarks = args.benchmarks or list(all)
    for b in args.benchmarks:
        if b not in all: parser.error(f"unknown benchmark {b}: choose among {', '.join(all)}")
    if not os.path.isfile(EXE): sys.exit(f"{EXE} not found: build it first (make release)")

    with tempfile.TemporaryDirectory(prefix="amosum_correctness_") as tmp:
        cases: List[Tuple[Case, List[Tuple[str, str, str, str]]]] = []
        rewritten: Dict[str, str] = {}
        for bench in args.benchmarks:
            files, configs = all[bench]
            for encoding, instance in files:
                with open(encoding) as f: text = f.read()
                case = Case(bench, encoding, instance, plain(text), "#amomaximize" in re.sub(r"%.*", "", text))
                if encoding not in rewritten: rewritten[encoding] = rewrite(encoding, tmp)
                case.rewritten = [rewritten[encoding]] + ([instance] if instance else [])
                base = [("nomin", "false")] if args.quick else configs
                # binary does not use --progression: once
                pairs = [(st, pr) for st in args.strategies for pr in (args.progressions if st != "binary" else ["1"])]
                if case.maximize: runs = [(st, pr, r, l) for st, pr in pairs for r, l in base]
                else:
                    runs = [("sat-unsat", "1", r, l) for r, l in base]
                    if bench == "edge": runs += [(st, pr, "nomin", "false") for st, pr in pairs if (st, pr) != ("sat-unsat", "1")]
                cases.append((case, runs))

        print(f"computing {len(cases)} references with clingo ...", flush=True)
        with ThreadPoolExecutor(args.jobs) as pool:
            refs = list(pool.map(lambda c: reference(c[0].plainText, c[0].instance, c[0].maximize), cases))
        for (case, _), ref in zip(cases, refs): case.ref = ref

        jobs = [(case, *run) for case, runs in cases for run in runs]
        print(f"running amoclingo-core {len(jobs)} times ...", flush=True)

        def one(job):
            case, strategy, progression, reason, lazy = job
            directory = tempfile.mkdtemp(dir=tmp)
            return job, check(case, runCore(case.rewritten, strategy, progression, reason, lazy, directory, args.timeout))

        failures = 0
        perCase: Dict[str, List[str]] = {}
        with ThreadPoolExecutor(args.jobs) as pool:
            for (case, strategy, progression, reason, lazy), errors in pool.map(one, jobs):
                if errors:
                    failures += 1
                    perCase.setdefault(case.name, []).append(f"  --opt-strategy={strategy} --progression={progression} --reason={reason} --lazy={lazy}: " + "; ".join(errors))
        for case, _ in cases:
            ref = case.ref
            status = "UNSAT" if not ref.sat else (f"OPTIMUM {ref.optimum}" if case.maximize else "SAT")
            print(f"{'FAIL' if case.name in perCase else 'ok  '} {case.name:70s} {status}")
            for line in perCase.get(case.name, []): print(line)

        # the wrapper: the options after the files, as a user writes them
        wrapperFailures = 0
        if not args.no_wrapper:
            wrapperCases = [c for c, _ in cases if c.bench in ("edge", "kn_toy_max")][:6]
            for case in wrapperCases:
                inputs = [case.encoding] + ([case.instance] if case.instance else [])
                errors = check(case, runWrapper(inputs, "min", "true", tempfile.mkdtemp(dir=tmp), args.timeout))
                print(f"{'FAIL' if errors else 'ok  '} wrapper {case.name}")
                for e in errors: print(f"  {e}")
                wrapperFailures += bool(errors)

        print(f"\namoclingo-core: {len(jobs) - failures}/{len(jobs)} runs correct"
              + ("" if args.no_wrapper else f"; wrapper: {len(wrapperCases) - wrapperFailures}/{len(wrapperCases)}"))
        return 1 if failures or wrapperFailures else 0


if __name__ == "__main__":
    sys.exit(main())
