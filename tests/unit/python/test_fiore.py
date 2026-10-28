"""fiore (Python): the pure Python parts, Result and the WASP plugin protocol"""
import unittest

from fiore.clingo.utility_clingo import ClingoResult as Result
from fiore.utils.constants import BOTTOM
from fiore.wasp.plugin import WaspPlugin
from fiore.wasp.runner import split_atoms


class TestResult(unittest.TestCase):

    def test_exit_codes(self):
        self.assertEqual(Result.satisfiable().exit_code(), 10)
        self.assertEqual(Result.unsatisfiable().exit_code(), 20)
        self.assertEqual(Result.satisfiable().set_optimum().exit_code(), 30)
        self.assertEqual(Result.satisfiable().set_unknown_optimum().exit_code(), 29)
        self.assertEqual(Result.unknown().exit_code(), 28)


class TestWasp(unittest.TestCase):

    def test_split_atoms(self):
        self.assertEqual(split_atoms("a, f(b,c), g((1,2),x), d"), ["a", "f(b,c)", "g((1,2),x)", "d"])
        self.assertEqual(split_atoms(""), [])

    def test_plugin_reasons_have_a_literal_of_the_current_level(self):
        """WASP wants a literal of the current decision level in every reason, and a conflict as a false literal"""

        class Engine:
            def __init__(self, derived, reasons):
                self.derived, self.reasons = derived, reasons
            def init_propagation(self): return []
            def on_literals_true(self, changes, dl, force=False): return self.derived
            def get_reason_for_literal(self, d): return self.reasons.get(d, [])
            def on_literals_undefined(self, changes): pass

        plugin = WaspPlugin({}, lambda names, level0: (Engine([-5], {-5: [3]}), [1, -1], []))
        plugin.get_literals(5)
        self.assertEqual(plugin.on_literal_true(2, 1), [-5])
        self.assertEqual(plugin.get_reason_for_literal(-5), [3, -2])      # not(2): of the current level

        plugin = WaspPlugin({}, lambda names, level0: (Engine([BOTTOM], {BOTTOM: [3]}), [1, -1], []))
        plugin.get_literals(5)
        self.assertEqual(plugin.on_literal_true(2, 1), [-2])              # the conflict: not(2), false
        self.assertEqual(plugin.get_reason_for_literal(-2), [3, -2])


if __name__ == "__main__":
    unittest.main()
