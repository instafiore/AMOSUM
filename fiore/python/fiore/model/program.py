"""A clingo program: grounding, its atoms by name and by program literal, extra ground rules (mirrors Program of
fiore/cpp/fiore/model/program.h)."""
from typing import Dict, Iterable, List, Optional

import clingo


class Literal:
    """A program atom: its program literal (> 0) and its name (the text of its symbol)."""
    __slots__ = ("id", "name", "symbol")

    def __init__(self, id: int, name: str, symbol: Optional[clingo.Symbol] = None):
        self.id = id
        self.name = name
        self.symbol = symbol

    def __repr__(self):
        return f"Literal({self.id}, {self.name})"


class Program:
    """The ground program of a clingo Control.

    `ground` loads the files and grounds the base part; `add_rules` grounds more (ground) rules in a new part, which
    may use the atoms of the previous parts in their bodies; `refresh` reads the atoms again."""

    # arguments of every Control: one model per solve call, #minimize not optimized (as fiore's C++ grounder)
    DEFAULT_ARGUMENTS = ["--models=1", "--opt-mode=ignore", "--warn=none"]

    def __init__(self, ctl: clingo.Control):
        self.ctl = ctl
        self.by_name: Dict[str, Literal] = {}
        self.by_id: Dict[int, Literal] = {}
        self.parts = 0

    @staticmethod
    def ground(files: Iterable[str], arguments: Iterable[str] = ()) -> "Program":
        ctl = clingo.Control(Program.DEFAULT_ARGUMENTS + list(arguments))
        for f in files:
            ctl.load(f)
        ctl.ground([("base", [])])
        program = Program(ctl)
        program.refresh()
        return program

    def add_rules(self, text: str) -> None:
        """Grounds `text` (ground rules, constraints, #show) in a new part."""
        if not text.strip():
            return
        self.parts += 1
        part = f"fiore_extra_{self.parts}"
        self.ctl.add(part, [], text)
        self.ctl.ground([(part, [])])
        self.refresh()

    def refresh(self) -> None:
        for atom in self.ctl.symbolic_atoms:
            name = str(atom.symbol)
            if name in self.by_name:
                continue
            literal = Literal(atom.literal, name, atom.symbol)
            self.by_name[name] = literal
            self.by_id.setdefault(atom.literal, literal)

    def literal(self, name: str) -> Optional[Literal]:
        return self.by_name.get(name)

    def atoms(self) -> List[Literal]:
        return list(self.by_name.values())
