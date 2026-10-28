"""The result of a solve call or of a search (mirrors ClingoResult of fiore/cpp/fiore/clingo/utilityClingo.h; pure
Python)."""
from fiore.utils.constants import OPTIMUM_CODE, OPTIMUM_UNKNOWN_CODE, SAT_CODE, UNKNOWN_CODE, UNSAT_CODE


class ClingoResult:
    """The outcome of a solve call or of a search: satisfiable or not (None: unknown), and for an optimization
    whether the optimum is proven."""

    def __init__(self, sat=None, optimum=False, unknown_optimum=False):
        self.sat = sat
        self.optimum = optimum
        self.unknown_optimum = unknown_optimum

    @staticmethod
    def satisfiable(): return ClingoResult(True)
    @staticmethod
    def unsatisfiable(): return ClingoResult(False)
    @staticmethod
    def unknown(): return ClingoResult(None)

    def is_sat(self): return self.sat is True
    def is_unsat(self): return self.sat is False
    def is_unknown(self): return self.sat is None

    def set_optimum(self):
        self.optimum, self.unknown_optimum = True, False
        return self

    def set_unknown_optimum(self):
        self.optimum, self.unknown_optimum = False, True
        return self

    def exit_code(self):
        if self.optimum: return OPTIMUM_CODE
        if self.unknown_optimum: return OPTIMUM_UNKNOWN_CODE if self.sat else UNKNOWN_CODE
        if self.sat is True: return SAT_CODE
        if self.sat is False: return UNSAT_CODE
        return UNKNOWN_CODE

    def __repr__(self):
        return f"ClingoResult(sat={self.sat}, optimum={self.optimum}, unknown_optimum={self.unknown_optimum})"
