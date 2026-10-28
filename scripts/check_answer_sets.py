#!/usr/bin/env python3
"""
Check that the answer sets printed by amoclingo / amowasp are answer sets of the
program (the encoding with #amosum / #eosum / #amomaximize plus its instance),
using the clingo API.

Usage:
    check_answer_sets.py --program encoding.asp --program instance.asp output.txt
    check_answer_sets.py --program encoding.asp,instance.asp -c n=3 output.txt
    check_answer_sets.py --program encoding.asp,instance.asp --check-unsat --check-optimum output.txt
    amoclingo encoding.asp instance.asp | check_answer_sets.py -p encoding.asp,instance.asp -

The output is expected in amoclingo's format (C++ and Python, also amowasp's):

    Amomaximize Cost: V Wall Time: Ts Time t Unkown AnswerSet N° : { 'a', 'b', ...}    a better model (#amomaximize)
    Time t [Optimum |Unkown ]AnswerSet N° : { 'a', 'b', ...}                           the last line: the best model
    UNSATISFIABLE | UNKNOWN | ... No solution found                                      or no model

A run killed at the time limit ends with the best model ("Wall Time ... AnswerSet")
or with "No solution found" / "UNKNOWN". Any other text (the "executing ..." line
of a wrapper script, the version line) is ignored.

The reference program
    The amosum statements are written as plain clingo, as in
    tests/correctness/check_correctness.py (set semantics, as amosum):
        #amosum{T1: C1 [G1]; ...} >= B : body.   ->   :- #sum{T1: C1; ...} < B, body.
        #eosum the same, #amomaximize{...}.      ->   #maximize{...}.

How an answer set is checked
    The program is grounded ONCE. Each printed answer set S is then checked by
    solving under assumptions, so nothing is grounded again.

    Mode "exact" (default when the program has no #show): every atom of the
    ground program is assumed true if it is in S, false otherwise. S passes if
    it is exactly an answer set; with #amomaximize its value must also be the
    printed "Amomaximize Cost". This is the right check for amoclingo, which
    prints all the atoms (except its own auxiliary atoms, starting with "__").

    Mode "constraint" (default with #show): one switch (an #external atom) per
    distinct output atom, adding ":- not a, switch."; S passes if some answer
    set of the program contains every atom of S. Atoms missing from the output
    are not detected.

    Mode "facts": a switch adds "a :- switch."; S passes if the program plus
    the atoms of S as facts is satisfiable. Weak: an atom the program would
    never derive is simply forced true.

Exit status: 0 whether or not the checks pass (the verdict is printed as
RESULT: PASSED / RESULT: FAILED); with --strict a failed check exits with 1.
An unparsable output file exits with 2; other errors (for example a missing
file) end with a Python traceback and exit status 1.
"""
import argparse
import re
import sys
from dataclasses import dataclass
from typing import Dict, List, Optional, Set, Tuple

import clingo

# Name of the external atoms used as switches. It starts with "__", so these
# atoms are also ignored in models by the default --ignore-prefix.
SWITCH = "__check_switch"


# ---------------------------------------------------------------------------
# Parsing the output
# ---------------------------------------------------------------------------
# [Amomaximize Cost: V ][Wall Time: Ts ][Time t ][Optimum |Unkown ]AnswerSet N° : { ... }
ANSWER_SET = re.compile(
    r"(?:Amomaximize Cost: (?P<value>-?\d+) )?(?P<wall>Wall Time: \S+ )?(?:Time \S+ )?"
    r"(?P<prefix>Optimum |Unkown )?AnswerSet (?P<number>\d+)° : \{(?P<atoms>.*)\}\s*$")


@dataclass
class Answer:
    number: int
    atoms: List[str]
    value: Optional[int]        # the printed "Amomaximize Cost" (None without #amomaximize)


def split_atoms(text: str) -> List[str]:
    """The atoms of "'a', 'p(\"x, y\")', ...": quoted with ', ignoring ' and , inside () and "" ."""
    atoms, cur, inside, depth, in_str, esc = [], [], False, 0, False, False
    for i, ch in enumerate(text):
        if not inside:
            if ch == "'":
                inside, cur = True, []
            continue
        if in_str:
            cur.append(ch)
            if esc:
                esc = False
            elif ch == "\\":
                esc = True
            elif ch == '"':
                in_str = False
        elif ch == '"':
            in_str = True
            cur.append(ch)
        elif ch == "(":
            depth += 1
            cur.append(ch)
        elif ch == ")":
            depth -= 1
            cur.append(ch)
        elif ch == "'" and depth == 0 and text[i + 1:].lstrip()[:1] in (",", ""):
            atoms.append("".join(cur))
            inside = False
        else:
            cur.append(ch)
    return atoms


def parse_output(text: str) -> Tuple[List[Answer], Optional[str]]:
    """Return (the printed answer sets, once each, in order; final status or None)."""
    answers: Dict[int, Answer] = {}
    status: Optional[str] = None
    for line in text.splitlines():
        line = line.strip()
        m = ANSWER_SET.search(line)
        if m:
            number = int(m.group("number"))
            value = int(m.group("value")) if m.group("value") is not None else None
            if number in answers:           # printed again: the final line, or the best one at an interruption
                if answers[number].value is None: answers[number].value = value
            else:
                answers[number] = Answer(number, split_atoms(m.group("atoms")), value)
            if not m.group("wall"):         # the final line of a run that was not interrupted
                prefix = (m.group("prefix") or "").strip()
                status = {"Optimum": "OPTIMUM FOUND", "Unkown": "UNKNOWN"}.get(prefix, "SATISFIABLE")
        elif line in ("UNSATISFIABLE", "UNKNOWN"):
            status = line
        elif line.endswith("No solution found"):
            status = "INTERRUPTED"
    if status is None and answers:
        status = "INTERRUPTED"              # amoclingo-core killed: the best model, without a final line
    return list(answers.values()), status


# ---------------------------------------------------------------------------
# The reference program: the amosum statements as plain clingo
# (the same translation as plain() in tests/correctness/check_correctness.py)
# ---------------------------------------------------------------------------
STATEMENT = re.compile(r"#(amosum|eosum|amomaximize)\s*\{")
BOUND = re.compile(r"\s*(<=|>=)\s*([^:.]+?)\s*(?::(.*?))?\.(?!\.)", re.S)     # >= B [: body].  ('..' is a range)
END = re.compile(r"\s*\.(?!\.)")


def split_top_level(text: str, separator: str) -> List[str]:
    parts, depth, current = [], 0, ""
    for ch in text:
        depth += ch in "([{"
        depth -= ch in ")]}"
        if ch == separator and depth == 0:
            parts.append(current)
            current = ""
        else:
            current += ch
    return parts + [current]


def plain(text: str) -> str:
    text = re.sub(r"%.*", "", text)
    out, pos = "", 0
    while True:
        m = STATEMENT.search(text, pos)
        if m is None:
            return out + text[pos:]
        out += text[pos:m.start()]
        i, depth = m.end(), 1                       # the matching }
        while depth:
            depth += text[i] == "{"
            depth -= text[i] == "}"
            i += 1
        elements = [re.sub(r"\[[^\[\]]*\]\s*$", "", e.strip()).strip() for e in split_top_level(text[m.end():i - 1], ";")]
        aggregate = "#sum{%s}" % "; ".join(e for e in elements if e)
        if m.group(1) == "amomaximize":
            end = END.match(text, i)
            out += "#maximize{%s}." % aggregate[5:-1]
        else:
            end = BOUND.match(text, i)
            if end is None:
                raise ValueError(f"cannot read the bound of {text[m.start():i + 40]!r}")
            op, bound, body = end.groups()
            out += ":- %s %s %s%s." % (aggregate, "<" if op == ">=" else ">", bound.strip(), (", " + body.strip()) if body else "")
        pos = end.end()


# ---------------------------------------------------------------------------
# Grounding (once) and checking
# ---------------------------------------------------------------------------
def ignored(symbol: clingo.Symbol, prefix: str) -> bool:
    return bool(prefix) and symbol.type == clingo.SymbolType.Function \
        and symbol.name.startswith(prefix)


def is_atom(symbol: clingo.Symbol) -> bool:
    """Numbers, strings and tuples can be printed but cannot be atoms."""
    return symbol.type == clingo.SymbolType.Function and symbol.name != ""


def _logger(code: clingo.MessageCode, message: str) -> None:
    """clingo's messages on stderr, except its notes on optimization: a check solves for a single model, on purpose"""
    if "optimality of last model not guaranteed" not in message and "optimize statement ignored" not in message:
        print(message, file=sys.stderr)


def value_of(model: clingo.Model) -> int:
    """The #amomaximize value of a model: clingo minimizes, so #maximize gives -value."""
    return -model.cost[0] if model.cost else 0


class Checker:
    """The program grounded once; in modes facts/constraint with one switch per distinct output atom."""

    def __init__(self, program: str, constants: List[str], atoms: List[clingo.Symbol], mode: str) -> None:
        self.mode = mode
        self.program, self.constants = program, constants
        self.ctl = clingo.Control(["1", "--warn=none"] + [x for c in constants for x in ("-c", c)], logger=_logger)
        self.ctl.add("base", [], program)

        rules = []
        if mode != "exact":
            for i, atom in enumerate(atoms):
                switch = f"{SWITCH}({i})"
                rules.append(f"#external {switch}.")
                if mode == "facts":
                    rules.append(f"{atom} :- {switch}.")
                else:
                    rules.append(f":- not {atom}, {switch}.")
        self.ctl.add("check", [], "\n".join(rules))
        self.ctl.ground([("base", []), ("check", [])])

        self.index: Dict[clingo.Symbol, int] = {a: i for i, a in enumerate(atoms)}
        self.switches = [clingo.Function(SWITCH, [clingo.Number(i)]) for i in range(len(atoms))] if mode != "exact" else []
        self.atom_of_literal: Dict[int, clingo.Symbol] = {}
        for i, switch in enumerate(self.switches):
            self.ctl.assign_external(switch, None)  # free: set by the assumptions
            self.atom_of_literal[self.ctl.symbolic_atoms[switch].literal] = atoms[i]
        # exact mode: the atoms of the ground program (the assumptions of every check)
        self.ground_atoms = [sa.symbol for sa in self.ctl.symbolic_atoms] if mode == "exact" else []
        for sa in (self.ctl.symbolic_atoms if mode == "exact" else []):
            self.atom_of_literal[sa.literal] = sa.symbol

    def _assumptions(self, on: Set) -> List[Tuple[clingo.Symbol, bool]]:
        if self.mode == "exact":
            return [(a, a in on) for a in self.ground_atoms]
        return [(switch, i in on) for i, switch in enumerate(self.switches)]

    def check(self, answer: Answer, prefix: str) -> Tuple[bool, List[str]]:
        """Return (passed, list of problems) for one printed answer set."""
        try:
            symbols = [clingo.parse_term(a) for a in answer.atoms]
        except RuntimeError as err:
            return False, [f"cannot parse an atom: {err}"]
        chosen = [s for s in symbols if not ignored(s, prefix)]
        not_atoms = sorted(str(s) for s in chosen if not is_atom(s))
        if not_atoms:
            return False, [f"{len(not_atoms)} term(s) of the output are not atoms{_sample(not_atoms)}"]

        if self.mode == "exact":
            known = set(self.ground_atoms)
            unknown = sorted(str(s) for s in chosen if s not in known)
            if unknown:
                return False, [f"{len(unknown)} atom(s) of the output are not in the ground program{_sample(unknown, 10)}"]
            on = set(chosen)
        else:
            on = {self.index[s] for s in chosen}

        core: List[int] = []
        values: List[int] = []
        result = self.ctl.solve(assumptions=self._assumptions(on),
                                on_model=lambda m: values.append(value_of(m)),
                                on_core=lambda c: core.extend(c))
        if result.satisfiable:
            # the value is reliable only in exact mode (the model is exactly S)
            if self.mode == "exact" and answer.value is not None and values and values[-1] != answer.value:
                return False, [f"printed Amomaximize Cost {answer.value}, but its value in the program is {values[-1]}"]
            return True, []

        what = "is not an answer set of the program" if self.mode == "exact" \
            else "the program is unsatisfiable together with the atoms of the output"
        problems = [what]
        described = sorted(str(self.atom_of_literal[lit]) for lit in core if lit in self.atom_of_literal)
        if described:
            problems.append(f"conflicting atoms (unsat core, {len(described)}){_sample(described, 10)}")
        return False, problems

    def satisfiable(self) -> bool:
        """Satisfiability of the original program (no assumptions)."""
        assumptions = [] if self.mode == "exact" else self._assumptions(set())
        return bool(self.ctl.solve(assumptions=assumptions).satisfiable)

    def optimum(self) -> Optional[int]:
        """The #amomaximize optimum of the program (None if unsatisfiable): a separate grounding."""
        ctl = clingo.Control(["--opt-mode=opt", "--warn=none"] + [x for c in self.constants for x in ("-c", c)], logger=_logger)
        ctl.add("base", [], self.program)
        ctl.ground([("base", [])])
        values: List[int] = []
        result = ctl.solve(on_model=lambda m: values.append(value_of(m)))
        return values[-1] if result.satisfiable and values else None

    def enumerate_all(self, prefix: str) -> Set[frozenset]:
        """All answer sets of the original program (shown atoms only; #maximize ignored)."""
        self.ctl.configuration.solve.models = "0"
        self.ctl.configuration.solve.opt_mode = "ignore"
        found: Set[frozenset] = set()
        assumptions = [] if self.mode == "exact" else self._assumptions(set())
        with self.ctl.solve(assumptions=assumptions, yield_=True) as handle:
            for model in handle:
                found.add(frozenset(s for s in model.symbols(shown=True) if not ignored(s, prefix)))
        self.ctl.configuration.solve.models = "1"
        self.ctl.configuration.solve.opt_mode = "opt"
        return found


def _sample(items: List[str], n: int = None) -> str:
    n = len(items) if n is None else n
    shown = ", ".join(items[:n])
    return f": {shown}{' ...' if len(items) > n else ''}"


# ---------------------------------------------------------------------------
# Main
# ---------------------------------------------------------------------------
def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__.split("\n\n")[0])
    ap.add_argument("output", help="amoclingo/amowasp output file to check ('-' for stdin)")
    ap.add_argument("-p", "--program", action="append", required=True, metavar="FILE[,FILE...]",
                    help="ASP file(s) of the encoding (with #amosum, ...) and instance; "
                         "repeatable or comma-separated")
    ap.add_argument("-c", "--const", action="append", default=[], metavar="ID=TERM",
                    help="constant for grounding, as in clingo")
    ap.add_argument("--ignore-prefix", default="__", metavar="P",
                    help="ignore atoms whose predicate starts with P (default: __, "
                         "amosum's auxiliary atoms; '' to disable)")
    ap.add_argument("--mode", choices=["auto", "exact", "constraint", "facts"], default="auto",
                    help="auto (default): exact without #show, constraint with #show; "
                         "see the docstring for the three checks")
    ap.add_argument("--strict", action="store_true",
                    help="exit with status 1 when a check fails (default: always exit 0)")
    ap.add_argument("--check-unsat", action="store_true",
                    help="when the output has no answer set, check its status against the "
                         "program (confirm UNSATISFIABLE)")
    ap.add_argument("--check-optimum", action="store_true",
                    help="with #amomaximize and status OPTIMUM FOUND, check the printed value "
                         "against clingo's optimum (solves the whole problem with clingo)")
    ap.add_argument("--complete", action="store_true",
                    help="also check that the output contains ALL answer sets of the program "
                         "(enumerates them: only for small programs without #amomaximize, "
                         "and only if all the answer sets were printed)")
    args = ap.parse_args()

    program_files = [f for p in args.program for f in p.split(",") if f]
    texts = []
    for f in program_files:
        with open(f) as fh:
            texts.append(fh.read())
    program = plain("\n".join(texts))
    maximize = "#maximize" in program
    mode = args.mode
    if mode == "auto":
        mode = "constraint" if re.search(r"#show\b", program) else "exact"

    if args.output == "-":
        text = sys.stdin.read()
    else:
        with open(args.output) as fh:
            text = fh.read()

    answers, status = parse_output(text)
    print(f"output: {len(answers)} answer set(s), status: {status or 'not found'}, mode: {mode}")
    if status is None and args.strict:
        return 1

    # modes facts/constraint: every distinct atom of the output gets a switch; then ground once
    atoms: List[clingo.Symbol] = []
    if mode != "exact":
        try:
            atoms = sorted({s for a in answers for s in map(clingo.parse_term, a.atoms)
                            if not ignored(s, args.ignore_prefix) and is_atom(s)})
        except RuntimeError as err:
            print(f"error: cannot parse an atom of the output: {err}", file=sys.stderr)
            return 2

    ok = True
    unsat_not_checked = not args.check_unsat and not answers
    if args.check_unsat or args.check_optimum or answers:
        checker = Checker(program, args.const, atoms, mode)
        if not answers:
            if args.check_unsat:
                sat = checker.satisfiable()
                if status == "UNSATISFIABLE" and sat:
                    ok = False
                    print("  output says UNSATISFIABLE, but the program has an answer set")
                elif status == "UNSATISFIABLE":
                    print("  UNSATISFIABLE confirmed")
                elif status in ("SATISFIABLE", "OPTIMUM FOUND"):
                    ok = False
                    print("  status says satisfiable but the output has no answer set to check")
        else:
            for answer in answers:
                good, problems = checker.check(answer, args.ignore_prefix)
                value = f", value {answer.value}" if answer.value is not None else ""
                if good:
                    print(f"  answer {answer.number}: OK ({len(answer.atoms)} atoms{value})")
                else:
                    ok = False
                    print(f"  answer {answer.number}: FAILED{value}")
                    for p in problems:
                        print(f"    - {p}")

            if maximize:
                values = [a.value for a in answers if a.value is not None]
                if any(b <= a for a, b in zip(values, values[1:])):
                    ok = False
                    print(f"  the printed values do not strictly increase: {values}")

            if args.check_optimum and maximize and status == "OPTIMUM FOUND":
                optimum = checker.optimum()
                best = answers[-1].value
                if optimum != best:
                    ok = False
                    print(f"  printed optimum {best}, but clingo's optimum is {optimum}")
                else:
                    print(f"  optimum {best} confirmed")

            if args.complete:
                if maximize:
                    print("complete check: skipped (with #amomaximize only the improving models are printed)")
                else:
                    expected = checker.enumerate_all(args.ignore_prefix)
                    got = {frozenset(s for s in map(clingo.parse_term, a.atoms) if not ignored(s, args.ignore_prefix))
                           for a in answers}
                    missing, extra = expected - got, got - expected
                    print(f"complete check: program has {len(expected)} answer set(s), output {len(got)}")
                    if missing:
                        ok = False
                        print(f"  {len(missing)} answer set(s) of the program missing from the output")
                    if extra:
                        ok = False
                        print(f"  {len(extra)} answer set(s) in the output are not answer sets")
                    if len(got) != len(answers):
                        print("  note: the output repeats some answer sets")

    result = "PASSED" if ok else "FAILED"
    result = f"{result} (unsat not checked)" if unsat_not_checked else result
    print(f"RESULT: {result}")
    return 1 if (args.strict and not ok) else 0


if __name__ == "__main__":
    sys.exit(main())
