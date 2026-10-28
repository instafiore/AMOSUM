"""The propagation of one amosum aggregate on solver literals (pure Python).

Port of the C++ AmoclingoPropagator::initFirstTime (the solving groups: build_solving_groups) and of AmosumPropagator
(AmosumEngine, one per solver thread). The engine follows the protocol of fiore.propagator / fiore.wasp:
on_true(changes, dl, force), undo(changes), reason(derived), init_check().

Difference from C++: both polarities of every member are watched, le_amo included, so the interpretation of the engine
is exact without reading the solver's assignment (which a WASP plugin cannot do).
"""
from typing import Callable, Dict, List, Optional, Sequence, Tuple

BOTTOM = 0              # the conflict (as fiore.propagator.BOTTOM)
TRUE = "true"           # a member decided before search (a fact)
FALSE = "false"         # a member that can never be true

REASON_IJCAI, REASON_MIN, REASON_MINFLY = "ijcai", "min", "minfly"


class Options:
    """--reason and --lazy (see AmoclingoPropagator)."""

    def __init__(self, reason: str = "nomin", lazy: str = "false"):
        self.full_true_reason = reason == REASON_IJCAI
        self.minimize_on_the_fly = reason in (REASON_MIN, REASON_MINFLY)
        self.lazy_active = lazy != "false"
        self.lazy_threshold = lazy in ("false", "hybrid")
        self.lazy_perc = 1.0 if lazy == "true" else (0.0 if self.lazy_threshold else float(lazy))


class ProgramGroupData:
    """A group of the aggregate at program level: members (program literal, weight); aux: the group of the "-" element."""

    def __init__(self, id: str, members: List[Tuple[int, int]], aux: bool = False):
        self.id, self.members, self.aux = id, members, aux


class SolvingGroup:
    def __init__(self, id: str, literals: List[int], moved: List[int], aux: bool):
        self.id, self.literals, self.moved, self.aux = id, literals, moved, aux


class SolvingGroups:
    """The groups of an aggregate over solver literals, shared by the threads (never modified during search)."""

    def __init__(self):
        self.constant = 0                       # weight of the groups decided by a fact (and of the negative rewriting)
        self.groups: List[SolvingGroup] = []
        self.weight: Dict[int, int] = {}        # member -> sum of the weights of its elements
        self.group_index: Dict[int, int] = {}   # member -> index of its owner group
        self.moved_into: Dict[int, List[int]] = {}   # moved literal -> the groups where it has weight 0
        self.watched: List[int] = []            # both polarities of every member and moved literal

    def is_moved(self, l: int) -> bool:
        return l in self.moved_into


def build_solving_groups(groups: List[ProgramGroupData], solver_literal: Callable[[int], object]) -> SolvingGroups:
    """Port of AmoclingoPropagator::initFirstTime (steps 1-4). solver_literal(program literal) is a solver literal,
    TRUE (decided before search) or FALSE (never true)."""
    s = SolvingGroups()
    n = len(groups)
    members: List[List[int]] = [[] for _ in range(n)]
    moved: List[List[int]] = [[] for _ in range(n)]
    moved_set: List[set] = [set() for _ in range(n)]
    owner: Dict[int, int] = {}
    merged: Dict[int, int] = {}
    decided = [False] * n

    # 1a. elements -> solver literals: equal solver literals are merged (weights summed) into the first group containing
    #     them; a later group containing the same solver literal keeps it as a moved literal (weight 0 there)
    for gi, g in enumerate(groups):
        for plit, w in g.members:
            slit = solver_literal(plit)
            if slit == FALSE:
                continue
            if slit == TRUE:
                decided[gi] = True
                s.constant += w
                continue
            if slit not in owner:
                owner[slit] = gi
                members[gi].append(slit)
            elif owner[slit] != gi and slit not in moved_set[gi]:
                moved_set[gi].add(slit)
                moved[gi].append(slit)
            merged[slit] = merged.get(slit, 0) + w

    # 1b. negative weights: w*s = |w|*not(s) - |w|: s becomes a moved literal of its group (worth 0 there), |w|*not(s) one
    #     more element (merged into the owner of not(s), in a new group if not owned), the constant takes -|w|
    ids = [g.id for g in groups]
    aux = [g.aux for g in groups]
    negatives = sorted(l for l, w in merged.items() if w < 0)
    moved_from: Dict[int, int] = {}
    while negatives:
        sl = negatives.pop()
        w = merged.get(sl)
        if w is None or w >= 0:
            continue
        gi = owner[sl]
        if sl not in moved_set[gi]:
            moved_set[gi].add(sl)
            moved[gi].append(sl)
        moved_from.setdefault(sl, gi)
        del owner[sl]
        del merged[sl]
        s.constant -= abs(w)
        ns = -sl
        if ns not in owner:
            if ns in moved_from:
                gk = moved_from.pop(ns)
                owner[ns] = gk
                moved_set[gk].discard(ns)
            else:
                owner[ns] = len(members)
                members.append([ns]); moved.append([]); moved_set.append(set())
                decided.append(False); ids.append(ids[gi] + "/not"); aux.append(False)
        merged[ns] = merged.get(ns, 0) + abs(w)

    # 2. a group with a fact is decided: by the AMO/EO promise its other elements are false
    for gi in range(len(members)):
        if decided[gi]:
            members[gi], moved[gi] = [], []

    # 3. groups over solver literals, ordered by (weight, literal); a group without own literals contributes 0
    for gi in range(len(members)):
        ms = moved_set[gi]
        group_members = [l for l in members[gi] if l not in ms]
        group_moved = [l for l in moved[gi] if l in ms]
        if not group_members:
            continue
        for l in group_members:
            s.weight[l] = merged[l]
        group_members.sort(key=lambda l: (s.weight[l], l))
        index = len(s.groups)
        for l in group_members:
            s.group_index[l] = index
        for l in group_moved:
            s.moved_into.setdefault(l, []).append(index)
        s.groups.append(SolvingGroup(ids[gi], group_members, group_moved, aux[gi]))

    # 4. watches: both polarities of every member and moved literal
    keys = set()
    for g in s.groups:
        for l in g.literals + g.moved:
            if abs(l) not in keys:
                keys.add(abs(l))
                s.watched += [l, -l]
    return s


class ThreadGroup:
    """Port of AmosumThreadGroup: one copy per thread, modified during search."""
    __slots__ = ("ordL", "ordI", "count_undef", "max_i", "min_i", "moved", "moved_not_false", "true_literal", "true_is_moved")

    def __init__(self, g: SolvingGroup):
        self.ordL = list(g.literals)
        self.ordI = {l: i for i, l in enumerate(self.ordL)}
        self.count_undef = len(self.ordL)
        self.max_i = len(self.ordL) - 1 if self.ordL else None
        self.min_i = 0 if self.ordL else None
        self.moved = g.moved
        self.moved_not_false = len(g.moved)
        self.true_literal = None
        self.true_is_moved = False

    def max_und(self): return None if self.max_i is None else self.ordL[self.max_i]
    def min_und(self): return None if self.min_i is None else self.ordL[self.min_i]

    def set_max_min(self, l, maximum: bool):
        i = None if l is None else self.ordI[l]
        if maximum: self.max_i = i
        else: self.min_i = i

    def update_max(self, undef, all: bool, update: bool, assuming=None):
        prev = self.max_und()
        start = len(self.ordL) - 1 if all else (self.max_i - 1 if self.max_i is not None else -1)
        for i in range(start, -1, -1):
            l = self.ordL[i]
            if undef(l) or (assuming is not None and abs(l) == abs(assuming)):
                if update: self.max_i = i
                return l, prev
        if update: self.max_i = None
        return None, prev

    def update_min(self, undef, all: bool, update: bool, assuming=None):
        prev = self.min_und()
        start = 0 if all else (self.min_i + 1 if self.min_i is not None else len(self.ordL))
        for i in range(start, len(self.ordL)):
            l = self.ordL[i]
            if undef(l) or (assuming is not None and abs(l) == abs(assuming)):
                if update: self.min_i = i
                return l, prev
        if update: self.min_i = None
        return None, prev

    def update(self, undef, maximum: bool, all: bool, update: bool, assuming=None):
        return self.update_max(undef, all, update, assuming) if maximum else self.update_min(undef, all, update, assuming)

    def remove(self, l, undef):
        """level 0: l is fixed for the whole search"""
        li = self.ordI.pop(l, None)
        if li is None:
            return
        for k in range(li + 1, len(self.ordL)):
            self.ordI[self.ordL[k]] -= 1
        self.ordL.pop(li)
        # recomputed from scratch below: the old indices may point past the shortened list
        self.max_i = self.min_i = None
        self.update_max(undef, True, True)
        self.update_min(undef, True, True)


class AmosumEngine:
    """Port of AmosumPropagator: one aggregate, one solver thread. `bound` reads the current bound (it may change
    between solve calls: the #amomaximize searches)."""

    def __init__(self, shared: SolvingGroups, ge: bool, eo: bool, options: Options, bound: Callable[[], int]):
        self.shared, self.ge, self.amo, self.options, self._bound = shared, ge, not eo, options, bound
        self.group_index = dict(shared.group_index)          # own copy: level 0 removes literals
        self.groups = [ThreadGroup(g) for g in shared.groups]
        self.value: Dict[int, bool] = {}                     # variable -> truth value of its positive literal
        self.current_sum = shared.constant
        self.mps_value = shared.constant + sum(self.contribution(g) for g in self.groups)
        self.max_diff = 0
        for g in shared.groups:
            if g.aux:
                continue                                     # changes only with the aggregate's body
            max_w = shared.weight[g.literals[-1]]
            min_w = shared.weight[g.literals[0]] if (options.lazy_active or not self.amo) and not g.moved else 0
            self.max_diff = max(self.max_diff, abs(max_w - min_w))
        self.decision_level = 0
        self.next_phase = False
        self.last_bound = bound()
        self.reasons: Dict[int, List[int]] = {}
        self.slack: Optional[Dict[int, int]] = {} if options.minimize_on_the_fly and ge else None
        self.S: List[int] = []

    # --- interpretation ---
    def is_true(self, l): v = self.value.get(abs(l)); return v is not None and v == (l > 0)
    def is_false(self, l): v = self.value.get(abs(l)); return v is not None and v != (l > 0)
    def is_undef(self, l): return abs(l) not in self.value

    def bound(self): return self._bound()
    def amo_le(self): return self.amo and not self.ge
    def mps(self): return self.current_sum if self.amo_le() else self.mps_value
    def w(self, l): return 0 if l is None else self.shared.weight[l]
    def is_member(self, l): return l in self.group_index

    def group_of(self, l):
        if l in self.group_index: return self.group_index[l]
        if -l in self.group_index: return self.group_index[-l]
        return None

    def contribution(self, g: ThreadGroup) -> int:
        """value of the group in the max (ge) / min (le) possible sum"""
        if g.true_literal is not None:
            return 0 if g.true_is_moved else self.w(g.true_literal)
        if not self.ge and g.moved_not_false > 0:
            return 0
        return self.w(g.max_und() if self.ge else g.min_und())

    def violated(self) -> bool:
        return self.mps() < self.bound() if self.ge else self.mps() > self.bound()

    def lazy_condition(self) -> bool:
        o, mps, b = self.options, self.mps(), self.bound()
        if o.lazy_threshold:
            return mps <= b + self.max_diff if self.ge else mps >= b - self.max_diff
        if self.ge:
            return mps <= 0 or b / mps >= o.lazy_perc
        return b <= 0 or mps / b >= o.lazy_perc

    # --- protocol ---
    def init_check(self) -> List[int]:
        if not self.violated():
            return []
        self.reasons[BOTTOM] = []
        return [BOTTOM]

    def reason(self, l: int) -> List[int]:
        return self.reasons.get(l, [])

    def on_true(self, changes: Sequence[int], dl: int, force: bool = False) -> List[int]:
        self.S = []
        if self.update_phase(changes, dl, force):
            return [BOTTOM]
        if self.next_phase:
            self.propagate_phase()
        return self.S

    def undo(self, changes: Sequence[int]) -> None:
        sh = self.shared
        for c in changes:
            if self.is_undef(c):
                continue
            member_true, member_false = self.is_member(c), self.is_member(-c)
            del self.value[abs(c)]
            if member_true: self.on_member_undefined(c)
            if member_false: self.on_member_undefined(-c)
            if sh.moved_into:
                for gi in sh.moved_into.get(c, ()):
                    g = self.groups[gi]
                    if g.true_literal != c:
                        continue
                    before = self.contribution(g)
                    g.true_literal, g.true_is_moved = None, False
                    self.mps_value += self.contribution(g) - before
                for gi in sh.moved_into.get(-c, ()):
                    g = self.groups[gi]
                    before = self.contribution(g)
                    g.moved_not_false += 1
                    self.mps_value += self.contribution(g) - before

    # --- update ---
    def update_phase(self, changes, dl, force) -> bool:
        """applies the changes; returns True on a conflict (the bound is violated)"""
        self.decision_level = dl
        changed = force and dl == 0
        if self.bound() != self.last_bound:
            self.last_bound = self.bound()
            changed = True
        sh = self.shared
        for c in changes:
            if self.is_true(c):
                continue
            self.value[abs(c)] = c > 0
            if self.is_member(c): changed |= self.on_member_true(c)
            if self.is_member(-c): changed |= self.on_member_false(-c)
            if sh.moved_into:
                for gi in sh.moved_into.get(c, ()): changed |= self.on_moved_true(gi, c)
                for gi in sh.moved_into.get(-c, ()): changed |= self.on_moved_false(gi)
            if dl == 0:
                self.simplify(c)
        if self.violated():
            self.reasons[BOTTOM] = []
            if self.slack is not None:
                self.slack[BOTTOM] = self.bound() - self.mps()
            if self.ge: self.explain_max_of_groups(BOTTOM, None)
            elif self.amo: self.explain_true_of_groups(BOTTOM, None)
            else: self.explain_min_of_groups(BOTTOM, None)
            self.next_phase = False
            return True
        self.next_phase = changed and self.lazy_condition() and (not self.ge or self.current_sum < self.bound())
        return False

    def on_member_true(self, l) -> bool:
        g = self.groups[self.group_index[l]]
        before = self.contribution(g)
        g.count_undef -= 1
        g.true_literal, g.true_is_moved = l, False
        self.current_sum += self.w(l)
        if g.count_undef == 0:
            if l == g.max_und(): g.max_i = None
            if l == g.min_und(): g.min_i = None
        after = self.contribution(g)
        self.mps_value += after - before
        return self.w(l) > 0 if self.amo_le() else before != after

    def on_member_false(self, x) -> bool:
        g = self.groups[self.group_index[x]]
        before = self.contribution(g)
        g.count_undef -= 1
        second, m = g.update(self.is_undef, self.ge, False, False, x)
        if x == m:
            g.set_max_min(second, self.ge)
            amo_condition = self.amo and g.true_literal is None
        elif x != second or not self.amo:
            return False
        else:
            amo_condition = True            # the second maximum changed: matters for the "must be true" derivation
        after = self.contribution(g)
        self.mps_value += after - before
        return not self.amo_le() and (amo_condition or before != after)

    def on_moved_true(self, gi, l) -> bool:
        g = self.groups[gi]
        if g.true_literal is not None:
            return False
        before = self.contribution(g)
        g.true_literal, g.true_is_moved = l, True
        after = self.contribution(g)
        self.mps_value += after - before
        return not self.amo_le() and before != after

    def on_moved_false(self, gi) -> bool:
        g = self.groups[gi]
        before = self.contribution(g)
        g.moved_not_false -= 1
        after = self.contribution(g)
        self.mps_value += after - before
        return not self.amo_le() and before != after

    def simplify(self, c):
        """level 0: the element of c leaves its group for good"""
        if self.is_member(c):
            self.decide(self.groups[self.group_index[c]], c)
        if self.is_member(-c):
            self.groups[self.group_index[-c]].remove(-c, self.is_undef)
            del self.group_index[-c]

    def decide(self, g: ThreadGroup, c):
        """level 0: c true decides g for good"""
        for l in g.ordL:
            self.group_index.pop(l, None)
        g.ordL, g.ordI, g.max_i, g.min_i = [], {}, None, None

    def on_member_undefined(self, l):
        g = self.groups[self.group_index[l]]
        before = self.contribution(g)
        g.count_undef += 1
        if g.true_literal == l:
            g.true_literal = None
            self.current_sum -= self.w(l)
        m = g.max_und() if self.ge else g.min_und()
        if m is None or (g.ordI[l] > g.ordI[m] if self.ge else g.ordI[l] < g.ordI[m]):
            g.set_max_min(l, self.ge)
        self.mps_value += self.contribution(g) - before

    # --- propagation ---
    def derive(self, l, mps_h=0):
        self.S.append(l)
        self.reasons[l] = []
        if self.slack is not None:
            self.slack[l] = self.bound() - mps_h

    def propagate_phase(self):
        if self.ge:
            self.propagate_ge_amo() if self.amo else self.propagate_ge_eo()
        else:
            self.propagate_le_amo() if self.amo else self.propagate_le_eo()

    def propagate_ge_amo(self):
        lb = self.bound()
        for g in self.groups:
            if g.true_literal is not None:
                continue
            ml = g.max_und()
            if ml is None:
                continue
            base = self.mps() - self.contribution(g)
            # without ml the group falls to its second maximum: if the bound is then lost, ml must be true
            second = g.update_max(self.is_undef, False, False)[0]
            mps_h = base + self.w(second)
            if mps_h < lb:
                self.derive(ml, mps_h)
                self.explain_max_without(ml, second, g)
                continue
            for l in g.ordL:                                # l true makes the group worth w(l): lighter first
                if not self.is_undef(l):
                    continue
                mps_h = base + self.w(l)
                if mps_h >= lb:
                    break
                self.derive(-l, mps_h)
        for d in list(self.S):
            self.explain_max_of_groups(d, self.group_of(d))

    def propagate_ge_eo(self):
        lb = self.bound()
        for g in self.groups:
            if g.true_literal is not None or g.max_und() is None:
                continue
            base = self.mps() - self.contribution(g)
            for l in g.ordL:
                if not self.is_undef(l):
                    continue
                mps_h = base + self.w(l)
                if mps_h >= lb:
                    break
                self.derive(-l, mps_h)
        for d in list(self.S):
            self.explain_max_of_groups(d, self.group_of(d))

    def propagate_le_amo(self):
        ub = self.bound()
        for g in self.groups:
            if g.true_literal is not None:
                continue
            for l in reversed(g.ordL):
                if not self.is_undef(l):
                    continue
                if self.current_sum + self.w(l) <= ub:
                    break
                self.derive(-l)
        for d in list(self.S):
            self.explain_true_of_groups(d, self.group_of(d))

    def propagate_le_eo(self):
        ub = self.bound()
        for g in self.groups:
            if g.true_literal is not None:
                continue
            base = self.mps() - self.contribution(g)
            for l in reversed(g.ordL):
                if not self.is_undef(l):
                    continue
                if base + self.w(l) <= ub:
                    break
                self.derive(-l)
        for d in list(self.S):
            self.explain_min_of_groups(d, self.group_of(d))

    # --- reasons: literals false in the current assignment ---
    def explain_max_of_groups(self, derived, excluded):
        """ge: each group can't exceed its current value; min/minfly leave out what the slack allows"""
        if self.decision_level == 0:
            return
        R, slack = self.reasons[derived], self.slack
        for gi, g in enumerate(self.groups):
            if gi == excluded:
                continue
            if g.true_literal is not None:
                if not g.ordL:
                    continue
                if slack is not None:
                    inc = self.w(g.ordL[-1]) - (0 if g.true_is_moved else self.w(g.true_literal))
                    if inc < slack[derived]:
                        slack[derived] -= inc
                        continue
                R.append(-g.true_literal)
                continue
            mw = self.w(g.max_und())
            for l in reversed(g.ordL):
                if self.w(l) <= mw:
                    break
                if not self.is_false(l):
                    continue
                if slack is not None and self.w(l) - mw < slack[derived]:
                    slack[derived] -= self.w(l) - mw
                    break
                R.append(l)

    def explain_max_without(self, derived, second, g: ThreadGroup):
        """ge: without the derived literal its group can't exceed the second maximum (ijcai: all its false literals)"""
        if self.decision_level == 0:
            return
        R, slack, full = self.reasons[derived], self.slack, self.options.full_true_reason
        i = g.ordI[second] if second is not None and not full else 0
        ws = self.w(second)
        for k in range(len(g.ordL) - 1, i - 1, -1):
            lit = g.ordL[k]
            if not full and self.w(lit) <= ws:
                break
            if not self.is_false(lit) or abs(lit) == abs(derived):
                continue
            if slack is not None and self.w(lit) - ws < slack[derived]:
                slack[derived] -= self.w(lit) - ws
                break
            R.append(lit)

    def explain_min_of_groups(self, derived, excluded):
        """le_eo: each group can't go below its current value"""
        if self.decision_level == 0:
            return
        R = self.reasons[derived]
        for gi, g in enumerate(self.groups):
            if gi == excluded:
                continue
            if g.true_literal is not None:
                if g.true_is_moved or not g.ordL:
                    continue
                R.append(-g.true_literal)
                continue
            if g.moved_not_false > 0:
                continue
            mw = self.w(g.min_und())
            for l in g.ordL:
                if self.w(l) > mw:
                    break
                if self.is_false(l):
                    R.append(l)
            if mw > 0:
                R.extend(g.moved)

    def explain_true_of_groups(self, derived, excluded):
        """le_amo: the true literals that make up the current sum"""
        if self.decision_level == 0:
            return
        R = self.reasons[derived]
        for gi, g in enumerate(self.groups):
            if gi == excluded:
                continue
            if g.true_literal is None or g.true_is_moved or not g.ordL:
                continue
            R.append(-g.true_literal)
