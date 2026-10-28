"""the searches with the Python propagator, on clingo (and on WASP if WASP_EXE is set)"""
import contextlib
import io
import os
import re
import unittest

from tests.unit.python.helpers import clingo_search, configurations, data


class TestSearchClingo(unittest.TestCase):

    def check_optimum(self, name, optimum):
        for strategy, progression in configurations():
            with self.subTest(strategy=strategy, progression=progression):
                best, result, values, _ = clingo_search(name, strategy, progression)
                self.assertTrue(result.optimum)
                self.assertEqual(best.value, optimum)
                self.assertEqual(values[-1], optimum)
                self.assertTrue(all(a < b for a, b in zip(values, values[1:])), values)

    def test_optimum_far_from_the_maximum(self):
        self.check_optimum("max_far", 17)

    def test_optimum_with_negative_weights(self):
        self.check_optimum("max_negative", -1)

    def test_level0_conflict_is_volatile(self):
        # a bound above 10 is violated at init: the conflict must hold for that solve call only
        self.check_optimum("max_level0", 10)

    def test_unsat_amomaximize(self):
        for strategy, progression in configurations():
            with self.subTest(strategy=strategy, progression=progression):
                self.assertTrue(clingo_search("max_unsat", strategy, progression)[1].is_unsat())

    def test_clause_type_of_the_amomaximize_propagator(self):
        self.assertFalse(clingo_search("max_far", "sat-unsat", "1")[3].maximizer.volatile)
        self.assertTrue(clingo_search("max_far", "sat-unsat", "2^n")[3].maximizer.volatile)
        self.assertTrue(clingo_search("max_far", "unsat-sat", "1")[3].maximizer.volatile)
        self.assertTrue(clingo_search("max_far", "binary")[3].maximizer.volatile)

    def test_without_amomaximize_is_one_solve_call(self):
        best, result, values, _ = clingo_search("ge_amo")
        self.assertTrue(result.is_sat() and not result.optimum)
        self.assertIn("c", best.shown)
        self.assertEqual(values, [])
        self.assertTrue(clingo_search("ge_amo_unsat")[1].is_unsat())

    def test_auxiliary_atoms_are_hidden(self):
        best, _, _, _ = clingo_search("shared_tuple")
        self.assertFalse(any(a.startswith("__") for a in best.shown))


@unittest.skipUnless(os.environ.get("WASP_EXE"), "WASP_EXE not set")
class TestSearchWasp(unittest.TestCase):
    """amowasp end to end on the .asp sources (it rewrites them): optimum and exit code"""

    def run_wasp(self, name, strategy="sat-unsat", progression="1"):
        from amosum.amowasp.python.runner import run
        out = io.StringIO()
        with contextlib.redirect_stdout(out):
            code = run([data(name, "asp")], strategy=strategy, progression=progression)
        values = [int(v) for v in re.findall(r"^Amomaximize Cost: (-?\d+) ", out.getvalue(), re.M)]
        return code, values, out.getvalue()

    def test_optima(self):
        for name, optimum in [("max_far", 17), ("max_level0", 10), ("max_negative", -1)]:
            for strategy, progression in [("sat-unsat", "1"), ("unsat-sat", "2^n"), ("binary", "1")]:
                with self.subTest(name=name, strategy=strategy, progression=progression):
                    code, values, _ = self.run_wasp(name, strategy, progression)
                    self.assertEqual(code, 30)
                    self.assertEqual(values[-1], optimum)

    def test_sat_and_unsat(self):
        self.assertEqual(self.run_wasp("ge_amo")[0], 10)
        self.assertEqual(self.run_wasp("ge_amo_unsat")[0], 20)
        self.assertEqual(self.run_wasp("shared_tuple")[0], 10)
        self.assertEqual(self.run_wasp("max_unsat")[0], 20)


if __name__ == "__main__":
    unittest.main()
