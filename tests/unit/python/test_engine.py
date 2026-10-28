"""amosum.core.engine without a solver: the solving groups and the propagation on hand-made solver literals"""
import unittest

from amosum.core.engine import (BOTTOM, FALSE, TRUE, AmosumEngine, Options, ProgramGroupData, build_solving_groups)


def identity(lit):
    return lit


def engine(groups, ge, eo, bound, solver_literal=identity, options=None):
    shared = build_solving_groups(groups, solver_literal)
    return AmosumEngine(shared, ge, eo, options or Options(), lambda: bound), shared


class TestSolvingGroups(unittest.TestCase):

    def test_groups_sorted_by_weight(self):
        shared = build_solving_groups([ProgramGroupData("g", [(1, 3), (2, 1), (3, 2)])], identity)
        self.assertEqual(shared.groups[0].literals, [2, 3, 1])
        self.assertEqual(shared.weight, {1: 3, 2: 1, 3: 2})

    def test_equal_solver_literal_is_merged_and_moved(self):
        # literal 5 in g1 and g2: its weights are summed in its first group, g2 keeps it as a moved literal (weight 0)
        shared = build_solving_groups([ProgramGroupData("g1", [(5, 2)]), ProgramGroupData("g2", [(5, 3), (6, 1)])], identity)
        self.assertEqual(shared.weight[5], 5)
        self.assertEqual(shared.group_index[5], 0)
        self.assertEqual(shared.groups[1].moved, [5])
        self.assertEqual(shared.moved_into, {5: [1]})

    def test_negative_weight_is_rewritten(self):
        # -2*s = 2*not(s) - 2: s moves to weight 0, not(s) gets 2 in a group of its own, the constant -2
        shared = build_solving_groups([ProgramGroupData("g", [(7, -2), (8, 1)])], identity)
        self.assertEqual(shared.constant, -2)
        self.assertEqual(shared.weight[-7], 2)
        self.assertIn(7, shared.groups[0].moved)

    def test_fact_decides_its_group(self):
        lits = {1: TRUE, 2: 2, 3: FALSE, 4: 4}
        shared = build_solving_groups([ProgramGroupData("g", [(1, 4), (2, 1)]), ProgramGroupData("h", [(3, 9), (4, 5)])],
                                      lambda l: lits[l])
        self.assertEqual(shared.constant, 4)                 # g decided by the fact, its other members out
        self.assertEqual([g.id for g in shared.groups], ["h"])
        self.assertEqual(shared.groups[0].literals, [4])     # 3 is never true

    def test_both_polarities_are_watched(self):
        shared = build_solving_groups([ProgramGroupData("g", [(1, 1), (2, 2)])], identity)
        self.assertEqual(sorted(shared.watched), [-2, -1, 1, 2])


class TestPropagation(unittest.TestCase):

    def test_ge_amo_conflict_and_reason(self):
        # g1 = {a:1, b:2}, g2 = {c:3}, >= 4: c false leaves at most 2
        e, _ = engine([ProgramGroupData("g1", [(1, 1), (2, 2)]), ProgramGroupData("g2", [(3, 3)])], True, False, 4)
        self.assertEqual(e.init_check(), [])
        self.assertEqual(e.on_true([-3], 1), [BOTTOM])
        self.assertEqual(e.reason(BOTTOM), [3])              # the clause: c (false now)

    def test_ge_amo_must_be_true_at_level_0(self):
        # one group {a:1, b:3}, >= 3: only b reaches the bound
        e, _ = engine([ProgramGroupData("g", [(1, 1), (2, 3)])], True, False, 3)
        self.assertEqual(e.on_true([], 0, force=True), [2])

    def test_violated_already_at_init(self):
        e, _ = engine([ProgramGroupData("g", [(1, 1)])], True, False, 2)
        self.assertEqual(e.init_check(), [BOTTOM])

    def test_le_amo_derivation_and_undo(self):
        # {a:3}, {b:2}, <= 4: a true makes b false, because of a
        e, _ = engine([ProgramGroupData("ga", [(1, 3)]), ProgramGroupData("gb", [(2, 2)])], False, False, 4)
        self.assertEqual(e.on_true([1], 1), [-2])
        self.assertEqual(e.reason(-2), [-1])
        e.undo([1])
        self.assertEqual(e.current_sum, 0)
        self.assertEqual(e.on_true([2], 1), [-1])            # b true (2): a (3) no longer fits under 4
        self.assertEqual(e.reason(-1), [-2])

    def test_le_eo_derivation(self):
        # EO groups {a:1, b:5}, {c:1, d:5}, <= 6: a true -> d false is not forced (1+5=6); b true -> d false
        e, _ = engine([ProgramGroupData("g1", [(1, 1), (2, 5)]), ProgramGroupData("g2", [(3, 1), (4, 5)])], False, True, 6)
        self.assertEqual(e.on_true([1], 1), [])
        e.undo([1])
        self.assertEqual(e.on_true([2], 1), [-4])

    def test_reason_ijcai_has_all_false_literals(self):
        # one group {a:1, b:2, c:3}, >= 2 with c false: b must be true; ijcai explains with every false literal
        e, _ = engine([ProgramGroupData("g", [(1, 1), (2, 2), (3, 3)])], True, False, 2, options=Options("ijcai"))
        derived = e.on_true([-3, -1], 1)
        self.assertIn(2, derived)
        self.assertEqual(sorted(e.reason(2)), [1, 3])


if __name__ == "__main__":
    unittest.main()
