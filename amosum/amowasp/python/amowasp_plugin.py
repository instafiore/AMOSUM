"""The WASP plugin of amowasp (loaded by WASP: --plugins-file=amowasp_plugin): one engine of amosum.core.engine per
aggregate, behind fiore's WaspPlugin. It runs inside WASP's own Python, so it imports only pure Python modules.

The aggregates (with the current bound of the #amomaximize) and the options come from the JSON file named by the
environment variable AMOWASP_CONFIG, written by amosum.amowasp.python.runner.
"""
import json
import os
import sys

_here = os.path.dirname(os.path.abspath(__file__))
_repo = os.path.abspath(os.path.join(_here, "..", "..", ".."))
for _path in (os.path.join(_repo, "fiore", "python"), _repo):
    if _path not in sys.path:
        sys.path.insert(0, _path)

from fiore.wasp import BOTTOM, WaspPlugin, install

from amosum.core.constraint import AmosumConstraint
from amosum.core.engine import FALSE, TRUE, AmosumEngine, Options, ProgramGroupData, build_solving_groups

# filled by WASP: atom name -> variable
atomNames = {}

with open(os.environ["AMOWASP_CONFIG"]) as _f:
    _config = json.load(_f)
_constraints = [AmosumConstraint.from_json(c) for c in _config["constraints"]]
_options = Options(_config["reason"], _config["lazy"])


class _Engines:
    """the engines of all the aggregates as one engine (WASP has one plugin)"""

    def __init__(self, engines):
        self.engines = engines
        self.reasons = {}

    def init_check(self):
        for e in self.engines:
            if BOTTOM in e.init_check():
                return [BOTTOM]
        return []

    def on_true(self, changes, dl, force=False):
        self.reasons = {}
        out = []
        for e in self.engines:
            derived = e.on_true(changes, dl, force)
            if BOTTOM in derived:
                self.reasons = {BOTTOM: list(e.reason(BOTTOM))}
                return [BOTTOM]
            for d in derived:
                if d not in self.reasons:
                    self.reasons[d] = list(e.reason(d))
                    out.append(d)
        return out

    def undo(self, changes):
        for e in self.engines:
            e.undo(changes)

    def reason(self, d):
        return self.reasons.get(d, [])


def _build(atom_names, level0):
    def solver_literal(member):
        name, positive = member
        var = atom_names.get(name)
        if var is None:
            return FALSE                        # not in the ground program (or not shown): never true
        lit = var if positive else -var
        if lit in level0:
            return TRUE
        if -lit in level0:
            return FALSE
        return lit

    engines, watched, seen = [], [], set()
    for c in _constraints:
        groups = [ProgramGroupData(g.id, [((m.name, m.positive), m.weight) for m in g.members], g.aux) for g in c.groups]
        shared = build_solving_groups(groups, solver_literal)
        engines.append(AmosumEngine(shared, c.ge, c.eo, _options, (lambda c=c: c.bound)))
        for l in shared.watched:
            if l not in seen:
                seen.add(l)
                watched.append(l)
    return _Engines(engines), watched, []


install(globals(), WaspPlugin(atomNames, _build))
