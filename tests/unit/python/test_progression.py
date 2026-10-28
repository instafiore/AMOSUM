"""--progression (amosum.core.py.search.progression): the same cases as the C++ unit tests"""
import unittest

from amosum.core.py.search.progression import CAP, Progression


def at(expression, n):
    return Progression(expression).at(n)


class TestProgression(unittest.TestCase):

    def test_constants(self):
        for n in range(5):
            self.assertEqual(at("1", n), 1)
            self.assertEqual(at("2", n), 2)

    def test_powers_and_polynomials(self):
        self.assertEqual([at("2^n", n) for n in range(6)], [1, 2, 4, 8, 16, 32])
        self.assertEqual(at("n^2", 3), 9)
        self.assertEqual(at("3*n+1", 4), 13)
        self.assertEqual(at("(n+1)^2", 2), 9)

    def test_values_below_one_count_as_one(self):
        self.assertEqual(at("n^2", 0), 1)
        self.assertEqual(at("-5", 0), 1)
        self.assertEqual(at("2^-1", 0), 1)
        self.assertEqual(at("n-10", 3), 1)

    def test_precedence(self):
        self.assertEqual(at("2+3*n", 2), 8)
        self.assertEqual(at("(2+3)*n", 2), 10)
        self.assertEqual(at("2*n^2", 3), 18)
        self.assertEqual(at("10+-2^2", 0), 6)          # -(2^2), not (-2)^2

    def test_associativity(self):
        self.assertEqual(at("10-3-2", 0), 5)
        self.assertEqual(at("2^n^2", 3), 512)          # 2^(3^2), not (2^3)^2

    def test_capped_at_2_to_62(self):
        self.assertEqual(at("2^n", 100), CAP)
        self.assertEqual(at("n^100", 10), CAP)
        self.assertEqual(at("99999999999999999999", 0), CAP)
        self.assertEqual(at("2^n*2^n", 40), CAP)

    def test_is_linear_only_for_constant_one(self):
        self.assertTrue(Progression("1").is_linear())
        self.assertTrue(Progression("2-1").is_linear())
        self.assertTrue(Progression("-5").is_linear())
        self.assertFalse(Progression("2").is_linear())
        self.assertFalse(Progression("2^n").is_linear())
        self.assertFalse(Progression("n^0").is_linear())

    def test_malformed_expressions_raise(self):
        for bad in ["", " ", "2^", "(n+1", "2 3", "x", "*2", "n)", "2**n", "+"]:
            with self.assertRaises(ValueError, msg=bad):
                Progression(bad)

    def test_error_message_has_the_position(self):
        with self.assertRaisesRegex(ValueError, "unexpected end at position 3"):
            Progression("2^")


if __name__ == "__main__":
    unittest.main()
