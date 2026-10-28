"""amosum.core.py.program: the aggregates read from the ground program and their normalization (needs clingo)"""
import unittest

from amosum.core.py.search.algorithms import max_possible_value, min_possible_value
from tests.unit.python.helpers import ground


def non_aux(c):
    return [g for g in c.groups if not g.aux]


class TestProgram(unittest.TestCase):

    def test_ge_amo_constraint(self):
        _, constraints = ground("ge_amo")
        self.assertEqual(len(constraints), 1)
        c = constraints[0]
        self.assertEqual((c.type, c.ge, c.bound), ("ge_amo", True, 4))
        weights = sorted(sorted(m.weight for m in g.members) for g in non_aux(c))
        self.assertEqual(weights, [[1, 2], [3]])
        self.assertEqual(len(c.groups) - len(non_aux(c)), 1)          # the aux group of the "-" element

    def test_le_eo_constraint(self):
        _, constraints = ground("le_eo")
        c = constraints[0]
        self.assertEqual((c.type, c.ge, c.bound, c.eo), ("le_eo", False, 1, True))
        self.assertEqual(sorted(m.weight for m in non_aux(c)[0].members), [1, 2, 3])

    def test_shared_tuple_counts_once(self):
        program, constraints = ground("shared_tuple")
        c = constraints[0]
        tuples = [g for g in c.groups if g.id.startswith("__tuple__")]
        self.assertEqual(len(tuples), 1)
        self.assertEqual([(m.name.startswith("__tuple__"), m.positive, m.weight) for m in tuples[0].members], [(True, True, 1)])
        for g in non_aux(c):
            if g is not tuples[0]:
                self.assertEqual([m.weight for m in g.members], [0])
        self.assertIsNotNone(program.literal(tuples[0].members[0].name))     # defined by the extra rules

    def test_equal_weights_in_a_group_are_merged(self):
        _, constraints = ground("same_weight")
        g = next(g for g in constraints[0].groups if g.id == "g")
        self.assertEqual(len(g.members), 1)
        self.assertTrue(g.members[0].name.startswith("__same__"))
        self.assertEqual(g.members[0].weight, 2)

    def test_amomaximize_and_possible_values(self):
        amomaximize = lambda constraints: next(c for c in constraints if c.is_amomaximize)
        _, far = ground("max_far")                                 # also a <= 17 aggregate
        self.assertEqual(sorted(c.type for c in far), ["amomaximize", "le_amo"])
        m = amomaximize(far)
        self.assertEqual((max_possible_value(m), min_possible_value(m)), (55, 0))
        m = amomaximize(ground("max_negative")[1])
        self.assertEqual((max_possible_value(m), min_possible_value(m)), (0, -3))

    def test_constraint_json_round_trip(self):
        from amosum.core.py.program import AmosumConstraint
        _, constraints = ground("shared_tuple")
        c = constraints[0]
        back = AmosumConstraint.from_json(c.to_json())
        self.assertEqual(back.to_json(), c.to_json())


if __name__ == "__main__":
    unittest.main()
