"""The WASP plugin of amowasp (loaded by WASP: --plugins-file=amowasp_plugin): one AmosumPropagator (core) per
aggregate, behind fiore's WaspPlugin; it plays the part of the clingo propagator (amoclingo/py/propagator.py). It runs
inside WASP's own Python, so it imports only pure Python modules; this folder is on WASP's sys.path, so it holds only
this module.

The aggregates (with the current bound of the #amomaximize) and the options come from the JSON file named by the
environment variable AMOWASP_CONFIG, written by amowasp/py/solver.py.
"""
import json
import os
import sys

_here = os.path.dirname(os.path.abspath(__file__))
_repo = os.path.abspath(os.path.join(_here, "..", "..", "..", ".."))
for _path in (os.path.join(_repo, "fiore", "python"), _repo):
    if _path not in sys.path:
        sys.path.insert(0, _path)

from fiore.model.propagator_pb_imp import PropagatorPBConstraintImplementation
from fiore.utils.constants import BOTTOM
from fiore.wasp.plugin import WaspPlugin, install

from amosum.core.py.program import AmosumConstraint
from amosum.core.py.propagator import FALSE, TRUE, AmosumOptions, AmosumPropagator, build_solving_groups

# filled by WASP: atom name -> variable
atomNames = {}

with open(os.environ["AMOWASP_CONFIG"]) as _f:
    _config = json.load(_f)
_constraints = [AmosumConstraint.from_json(c) for c in _config["constraints"]]
_options = AmosumOptions(_config["reason"], _config["lazy"])


class _Propagators(PropagatorPBConstraintImplementation):
    """the propagators of all the aggregates as one (WASP has one plugin)"""

    def __init__(self, propagators):
        self.propagators = propagators
        self.reasons = {}

    def init_propagation(self):
        for p in self.propagators:
            if BOTTOM in p.init_propagation():
                self.reasons = {BOTTOM: list(p.get_reason_for_literal(BOTTOM))}
                return [BOTTOM]
        return []

    def on_literals_true(self, changes, dl, force=False):
        self.reasons = {}
        out = []
        for p in self.propagators:
            derived = p.on_literals_true(changes, dl, force)
            if BOTTOM in derived:
                self.reasons = {BOTTOM: list(p.get_reason_for_literal(BOTTOM))}
                return [BOTTOM]
            for d in derived:
                if d not in self.reasons:
                    self.reasons[d] = list(p.get_reason_for_literal(d))
                    out.append(d)
        return out

    def on_literals_undefined(self, changes):
        for p in self.propagators:
            p.on_literals_undefined(changes)

    def get_reason_for_literal(self, d):
        return self.reasons.get(d, [])


def _build(atom_names, level0):
    def solver_literal(member):
        var = atom_names.get(member.name)
        if var is None:
            return FALSE                        # not in the ground program (or not shown): never true
        lit = var if member.positive else -var
        if lit in level0:
            return TRUE
        if -lit in level0:
            return FALSE
        return lit

    propagators, watched, seen = [], [], set()
    for c in _constraints:
        solving_groups = build_solving_groups(c, solver_literal)
        propagators.append(AmosumPropagator(solving_groups, c.ge, c.eo, _options, (lambda c=c: c.bound)))
        for l in solving_groups.watched:
            if l not in seen:
                seen.add(l)
                watched.append(l)
    return _Propagators(propagators), watched, []


install(globals(), WaspPlugin(atomNames, _build))
