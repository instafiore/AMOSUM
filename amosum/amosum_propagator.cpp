#include "amosum_propagator.h"
#include "amosum_solver.h"
#include <algorithm>

// ----- AmosumGroup -----

AmosumThreadGroup::AmosumThreadGroup(const AmosumSolvingGroup& g) noexcept:
    ordL(g.literals), countUndef(g.literals.size()),
    maxUndI(g.literals.empty() ? NONE_INDEX : g.literals.size() - 1), minUndI(g.literals.empty() ? NONE_INDEX : 0),
    movedLiterals(&g.movedLiterals), movedNotFalse(g.movedLiterals.size()), trueLiteral(SETTINGS::NONE_LITERAL), trueIsMoved(false){
    for(size_t i = 0; i < ordL.size(); ++i) ordI[ordL[i]] = i;
}

std::pair<clingo_literal_t, clingo_literal_t> AmosumThreadGroup::updateMax(const fiore::PerfectInterpretation& I, bool all, bool update, clingo_literal_t assumingUnd) noexcept{
    const clingo_literal_t prev = maxUnd();
    const int64_t start = all ? (int64_t)ordL.size() - 1 : (maxUndI != NONE_INDEX ? (int64_t)maxUndI - 1 : -1);
    for(int64_t i = start; i >= 0; --i){
        const clingo_literal_t l = ordL[i];
        if(I.isUndef(l) || (assumingUnd != SETTINGS::NONE_LITERAL && std::abs(l) == std::abs(assumingUnd))){
            if(update) maxUndI = i;
            return {l, prev};
        }
    }
    if(update) maxUndI = NONE_INDEX;
    return {SETTINGS::NONE_LITERAL, prev};
}

std::pair<clingo_literal_t, clingo_literal_t> AmosumThreadGroup::updateMin(const fiore::PerfectInterpretation& I, bool all, bool update, clingo_literal_t assumingUnd) noexcept{
    const clingo_literal_t prev = minUnd();
    const size_t start = all ? 0 : (minUndI != NONE_INDEX ? minUndI + 1 : ordL.size());
    for(size_t i = start; i < ordL.size(); ++i){
        const clingo_literal_t l = ordL[i];
        if(I.isUndef(l) || (assumingUnd != SETTINGS::NONE_LITERAL && std::abs(l) == std::abs(assumingUnd))){
            if(update) minUndI = i;
            return {l, prev};
        }
    }
    if(update) minUndI = NONE_INDEX;
    return {SETTINGS::NONE_LITERAL, prev};
}

void AmosumThreadGroup::remove(const clingo_literal_t l, const fiore::PerfectInterpretation& I) noexcept{
    auto it = ordI.find(l);
    if(it == ordI.end()) return;
    const size_t li = it->second;
    ordI.erase(it);
    for(size_t k = li + 1; k < ordL.size(); ++k) ordI[ordL[k]] -= 1;
    ordL.erase(ordL.begin() + li);
    updateMax(I, true, true);
    updateMin(I, true, true);
}

// ----- AmosumPropagator -----

AmosumPropagator::AmosumPropagator(AmoclingoPropagator* abs) noexcept:
    fiore::PropagatorPBConstraintImplementation(abs), prop(abs), ge(abs->ge), amo(!abs->eo),
    ownedGroupIndex(abs->solver->initializer->nt > 1 ? new fiore::PerfectNHash<clingo_literal_t, size_t>(abs->N, AmoclingoPropagator::NO_GROUP) : nullptr),
    groupIndex(ownedGroupIndex ? ownedGroupIndex.get() : abs->groupIndex.get()),
    I(new fiore::PerfectInterpretation(abs->N, fiore::TruthValue(fiore::TruthValue::U))),
    currentSum(abs->constant), mpsValue(abs->constant), maxDiff(0), decisionLevel(0), nextPhase(false),
    reasons(new fiore::PerfectNHash<clingo_literal_t, std::vector<clingo_literal_t>>(abs->N)),
    slack(abs->minimizeOnTheFly && abs->ge ? new fiore::PerfectNHash<clingo_literal_t, int64_t>(abs->N, 0) : nullptr){

    groups.reserve(abs->solvingGroups.size());
    for(size_t gi = 0; gi < abs->solvingGroups.size(); ++gi){
        const AmosumSolvingGroup& g = abs->solvingGroups[gi];
        groups.emplace_back(g);
        // deep copy: PerfectNHash owns its array, the implicit copy would share it
        if(ownedGroupIndex) for(const clingo_literal_t l: g.literals) ownedGroupIndex->set(l, gi);
    }
    for(const AmosumThreadGroup& g: groups) mpsValue += contribution(g);

    // as in the old specific_phase: the largest change of the possible sum one group can cause, aux group excluded
    // (with lazy active the minimum weight of the group is used for AMO too)
    for(const AmosumSolvingGroup& g: abs->solvingGroups){
        if(g.literals.size() == 1 && abs->auxes.count(g.literals[0]) > 0) continue;
        const int64_t maxW = weightOf(g.literals.back());
        const int64_t minW = (abs->lazyActive || !amo) && g.movedLiterals.empty() ? weightOf(g.literals.front()) : 0;
        maxDiff = std::max(maxDiff, std::abs(maxW - minW));
    }
}

// value of the group in the max (ge) / min (le) possible sum
int64_t AmosumPropagator::contribution(const AmosumThreadGroup& g) const noexcept{
    if(g.trueLiteral != SETTINGS::NONE_LITERAL) return g.trueIsMoved ? 0 : weightOf(g.trueLiteral);
    if(!ge && g.movedNotFalse > 0) return 0;      // a moved literal may still be the true element of the group
    return weightOf(ge ? g.maxUnd() : g.minUnd());
}

// old update_lazy_propagation: p >= lazy_perc with p = lb/mps (ge), mps/ub (le); false and hybrid use the threshold
// lb/(lb+maxDiff) (ge), (ub-maxDiff)/ub (le), i.e. a single group can still bring the possible sum across the bound
bool AmosumPropagator::lazyCondition() const noexcept{
    if(prop->lazyThreshold) return ge ? mps() <= bound() + maxDiff : mps() >= bound() - maxDiff;
    if(ge) return mps() <= 0 || (double)bound() / (double)mps() >= prop->lazyPerc;
    return bound() <= 0 || (double)mps() / (double)bound() >= prop->lazyPerc;
}

const std::vector<clingo_literal_t>& AmosumPropagator::getReasonForLiteral(const clingo_literal_t& literalId) const noexcept{
    return reasons->at(literalId);
}

// ----- update -----

std::pair<bool, bool> AmosumPropagator::updatePhase(clingo_literal_t const *literalIds, size_t size, size_t dl) noexcept{
    decisionLevel = dl;
    bool changed = false;       // sumChanged || amo_condition of the old update_phase, for some change
    for(size_t i = 0; i < size; ++i){
        const clingo_literal_t c = literalIds[i];
        // literal 1, true from the start: first propagation, as the old simplifyAtLevelZero
        if(c == fiore::CONSTANTS::TOP){ changed |= dl == 0; continue; }
        if(I->isTrue(c)) continue;
        I->set(c, fiore::TruthValue::T);
        // c true: c may be a member (true), the negation of a member (false), a moved literal of other groups
        if(isMemberHere(c)) changed |= onMemberTrue(c);
        if(isMemberHere(fiore::not_(c))) changed |= onMemberFalse(fiore::not_(c));
        if(prop->hasMovedLiterals){
            if(auto it = prop->movedInto.find(c); it != prop->movedInto.end()) for(const size_t gi: it->second) changed |= onMovedTrue(gi, c);
            if(auto it = prop->movedInto.find(fiore::not_(c)); it != prop->movedInto.end()) for(const size_t gi: it->second) changed |= onMovedFalse(gi);
        }
        if(dl == 0) simplify(c);
    }

    // the whole change set is applied before propagating: a violated bound is a conflict explained by the current assignment
    if(ge ? mps() < bound() : mps() > bound()){
        const clingo_literal_t bottom = fiore::CONSTANTS::BOTTOM;
        reasons->get(bottom).clear();
        if(slack) slack->set(bottom, bound() - mps());
        if(ge) explainMaxOfGroups(bottom, AmoclingoPropagator::NO_GROUP);
        else if(amo) explainTrueOfGroups(bottom, AmoclingoPropagator::NO_GROUP);
        else explainMinOfGroups(bottom, AmoclingoPropagator::NO_GROUP);
        nextPhase = false;
        return {false, true};
    }
    nextPhase = changed && lazyCondition() && (!ge || currentSum < bound());
    return {true, false};
}

bool AmosumPropagator::onMemberTrue(const clingo_literal_t l) noexcept{
    AmosumThreadGroup& g = groups[groupIndex->at(l)];
    const int64_t before = contribution(g);
    assert(g.countUndef > 0);
    g.countUndef--;
    g.trueLiteral = l;
    g.trueIsMoved = false;
    currentSum += weightOf(l);
    if(g.countUndef == 0){
        if(l == g.maxUnd()) g.maxUndI = AmosumThreadGroup::NONE_INDEX;
        if(l == g.minUnd()) g.minUndI = AmosumThreadGroup::NONE_INDEX;
    }
    const int64_t after = contribution(g);
    mpsValue += after - before;
    return amoLe() ? weightOf(l) > 0 : before != after;
}

bool AmosumPropagator::onMemberFalse(const clingo_literal_t x) noexcept{
    AmosumThreadGroup& g = groups[groupIndex->at(x)];
    const int64_t before = contribution(g);
    assert(g.countUndef > 0);
    g.countUndef--;
    // x considered undefined in the scan, as in the old update_phase
    const auto [second, m] = g.update(*I, ge, false, false, x);
    bool amoCondition;
    if(x == m){
        g.setMaxMin(second, ge);
        amoCondition = amo && g.trueLiteral == SETTINGS::NONE_LITERAL;
    }else if(x != second || !amo){
        return false;
    }else{
        amoCondition = true;        // the second maximum changed: matters for the "must be true" derivation
    }
    const int64_t after = contribution(g);
    mpsValue += after - before;
    return !amoLe() && (amoCondition || before != after);
}

bool AmosumPropagator::onMovedTrue(const size_t gi, const clingo_literal_t l) noexcept{
    AmosumThreadGroup& g = groups[gi];
    if(g.trueLiteral != SETTINGS::NONE_LITERAL) return false;
    const int64_t before = contribution(g);
    g.trueLiteral = l;
    g.trueIsMoved = true;
    const int64_t after = contribution(g);
    mpsValue += after - before;
    return !amoLe() && before != after;
}

bool AmosumPropagator::onMovedFalse(const size_t gi) noexcept{
    AmosumThreadGroup& g = groups[gi];
    const int64_t before = contribution(g);
    assert(g.movedNotFalse > 0);
    g.movedNotFalse--;
    const int64_t after = contribution(g);
    mpsValue += after - before;
    return !amoLe() && before != after;
}

// level 0: the element of c leaves its group for good, and c is not watched any more if nothing else needs it
void AmosumPropagator::simplify(const clingo_literal_t c) noexcept{
    // c true: its group is decided (at most one true literal), all its literals leave it
    if(isMemberHere(c)) decide(groups[groupIndex->at(c)], c);
    // the negation of c false: only that literal leaves its group
    if(isMemberHere(fiore::not_(c))){
        groups[groupIndex->at(fiore::not_(c))].remove(fiore::not_(c), *I);
        groupIndex->set(fiore::not_(c), AmoclingoPropagator::NO_GROUP);
    }
    if(!isMemberHere(c) && !isMemberHere(fiore::not_(c)) && !prop->isMoved(c) && !prop->isMoved(fiore::not_(c))){
        watchesToRemove.push_back(c);
        watchesToRemove.push_back(fiore::not_(c));
    }
}

// level 0: c true decides g for good: its value is w(c) (trueLiteral), its other literals can no longer change anything
void AmosumPropagator::decide(AmosumThreadGroup& g, const clingo_literal_t c) noexcept{
    for(const clingo_literal_t l: g.ordL) groupIndex->set(l, AmoclingoPropagator::NO_GROUP);
    for(const clingo_literal_t l: g.ordL){
        if(l == c) continue;                           // c: checked by simplify
        if(!isMemberHere(fiore::not_(l)) && !prop->isMoved(l) && !prop->isMoved(fiore::not_(l))){
            watchesToRemove.push_back(l);
            watchesToRemove.push_back(fiore::not_(l));
        }
    }
    g.ordL.clear();
    g.ordI.clear();
    g.maxUndI = AmosumThreadGroup::NONE_INDEX;
    g.minUndI = AmosumThreadGroup::NONE_INDEX;
}

// ----- undo -----

void AmosumPropagator::onLiteralsUndefined(clingo_propagate_control_t const *control, clingo_literal_t const *literalIds, size_t size, size_t dl) noexcept{
    for(size_t i = 0; i < size; ++i){
        const clingo_literal_t c = literalIds[i];
        if(c == fiore::CONSTANTS::TOP || I->isUndef(c)) continue;
        const bool memberTrue = isMemberHere(c);
        const bool memberFalse = isMemberHere(fiore::not_(c));
        I->set(c, fiore::TruthValue::U);
        if(memberTrue) onMemberUndefined(c);
        if(memberFalse) onMemberUndefined(fiore::not_(c));
        if(prop->hasMovedLiterals){
            if(auto it = prop->movedInto.find(c); it != prop->movedInto.end()){
                for(const size_t gi: it->second){
                    AmosumThreadGroup& g = groups[gi];
                    if(g.trueLiteral != c) continue;
                    const int64_t before = contribution(g);
                    g.trueLiteral = SETTINGS::NONE_LITERAL;
                    g.trueIsMoved = false;
                    mpsValue += contribution(g) - before;
                }
            }
            if(auto it = prop->movedInto.find(fiore::not_(c)); it != prop->movedInto.end()){
                for(const size_t gi: it->second){
                    AmosumThreadGroup& g = groups[gi];
                    const int64_t before = contribution(g);
                    g.movedNotFalse++;
                    mpsValue += contribution(g) - before;
                }
            }
        }
    }
}

void AmosumPropagator::onMemberUndefined(const clingo_literal_t l) noexcept{
    AmosumThreadGroup& g = groups[groupIndex->at(l)];
    const int64_t before = contribution(g);
    g.countUndef++;
    if(g.trueLiteral == l){
        g.trueLiteral = SETTINGS::NONE_LITERAL;
        currentSum -= weightOf(l);
    }
    // l becomes the maximum (minimum) undefined literal if it comes after (before) the current one, as in the old onLiteralsUndefined
    const clingo_literal_t m = ge ? g.maxUnd() : g.minUnd();
    if(m == SETTINGS::NONE_LITERAL || (ge ? g.ordI.at(l) > g.ordI.at(m) : g.ordI.at(l) < g.ordI.at(m))) g.setMaxMin(l, ge);
    mpsValue += contribution(g) - before;
}

// ----- propagation -----

void AmosumPropagator::derive(const clingo_literal_t l, const int64_t mpsH) noexcept{
    S.push_back(l);
    reasons->get(l).clear();
    // the derivation holds as long as the possible sum stays below lb: what is left out of the reason may raise it by
    // less than lb - mpsH
    if(slack) slack->set(l, bound() - mpsH);
}


std::vector<clingo_literal_t>& AmosumPropagator::propagatePhase(clingo_propagate_control_t* control) noexcept{
    // literals fixed at level 0 left the groups for good: their watches are not needed in this thread any more
    for(const clingo_literal_t l: watchesToRemove) clingo_propagate_control_remove_watch(control, l);
    watchesToRemove.clear();

    if(!nextPhase) return S;
    // I is up to date for every watched literal: both polarities are watched except in le_amo
    if(ge) amo ? propagateGeAmo() : propagateGeEo();
    else amo ? propagateLeAmo(clingo_propagate_control_assignment(control)) : propagateLeEo();
    return S;
}

// port of propagation_phase_ge_amo
void AmosumPropagator::propagateGeAmo() noexcept{
    const int64_t lb = bound();
    for(AmosumThreadGroup& g: groups){
        if(g.trueLiteral != SETTINGS::NONE_LITERAL) continue;
        const clingo_literal_t ml = g.maxUnd();
        if(ml == SETTINGS::NONE_LITERAL) continue;
        const int64_t base = mps() - contribution(g);
        // without ml the group falls to its second maximum: if the bound is then lost, ml must be true
        const clingo_literal_t second = g.updateMax(*I, false, false).first;
        const int64_t mpsH = base + weightOf(second);
        if(mpsH < lb){
            derive(ml, mpsH);
            explainMaxWithout(ml, second, g);
            continue;
        }
        // l true makes the group worth w(l): lighter literals first
        for(const clingo_literal_t l: g.ordL){
            if(!I->isUndef(l)) continue;
            const int64_t mpsH = base + weightOf(l);
            if(mpsH >= lb) break;
            derive(fiore::not_(l), mpsH);
        }
    }
    for(const clingo_literal_t d: S) explainMaxOfGroups(d, groupOf(d));
}

// port of propagation_phase_ge_eo
void AmosumPropagator::propagateGeEo() noexcept{
    const int64_t lb = bound();
    for(AmosumThreadGroup& g: groups){
        if(g.trueLiteral != SETTINGS::NONE_LITERAL || g.maxUnd() == SETTINGS::NONE_LITERAL) continue;
        const int64_t base = mps() - contribution(g);
        for(const clingo_literal_t l: g.ordL){
            if(!I->isUndef(l)) continue;
            const int64_t mpsH = base + weightOf(l);
            if(mpsH >= lb) break;
            derive(fiore::not_(l), mpsH);
        }
    }
    for(const clingo_literal_t d: S) explainMaxOfGroups(d, groupOf(d));
}

// port of propagation_phase_le_amo
void AmosumPropagator::propagateLeAmo(const clingo_assignment_t* assignment) noexcept{
    const int64_t ub = bound();
    for(AmosumThreadGroup& g: groups){
        if(g.trueLiteral != SETTINGS::NONE_LITERAL) continue;
        for(size_t i = g.ordL.size(); i-- > 0;){
            const clingo_literal_t l = g.ordL[i];
            // the negations are not watched in le_amo: l may be false in the solver and undefined in I
            if(!I->isUndef(l) || fiore::clingo::isFalse(assignment, l)) continue;
            if(currentSum + weightOf(l) <= ub) break;
            derive(fiore::not_(l));
        }
    }
    for(const clingo_literal_t d: S) explainTrueOfGroups(d, groupOf(d));
}

// port of propagation_phase_le_eo
void AmosumPropagator::propagateLeEo() noexcept{
    const int64_t ub = bound();
    for(AmosumThreadGroup& g: groups){
        if(g.trueLiteral != SETTINGS::NONE_LITERAL) continue;
        const int64_t base = mps() - contribution(g);
        for(size_t i = g.ordL.size(); i-- > 0;){
            const clingo_literal_t l = g.ordL[i];
            if(!I->isUndef(l)) continue;
            if(base + weightOf(l) <= ub) break;
            derive(fiore::not_(l));
        }
    }
    for(const clingo_literal_t d: S) explainMinOfGroups(d, groupOf(d));
}

// ----- reasons -----

// ge (port of create_reason_falses_ge): each group can't exceed its current value; min/minfly leave out what the slack allows
void AmosumPropagator::explainMaxOfGroups(const clingo_literal_t derived, const size_t excluded) noexcept{
    if(decisionLevel == 0) return;
    std::vector<clingo_literal_t>& R = reasons->get(derived);
    for(size_t gi = 0; gi < groups.size(); ++gi){
        if(gi == excluded) continue;
        const AmosumThreadGroup& g = groups[gi];
        // decided group: its value is the weight of the true literal
        if(g.trueLiteral != SETTINGS::NONE_LITERAL){
            // ordL emptied at level 0 (decide, or all literals false): the value of the group is fixed, nothing to explain
            if(g.ordL.empty()) continue;
            // without it the group reaches its heaviest literal
            if(slack){
                const int64_t inc = weightOf(g.ordL.back()) - (g.trueIsMoved ? 0 : weightOf(g.trueLiteral));
                if(inc < slack->get(derived)){ slack->get(derived) -= inc; continue; }
            }
            R.push_back(fiore::not_(g.trueLiteral));
            continue;
        }
        // otherwise its value is the maximum undefined literal: the heavier literals are false
        const int64_t mw = weightOf(g.maxUnd());
        for(size_t i = g.ordL.size(); i-- > 0;){
            const clingo_literal_t l = g.ordL[i];
            if(weightOf(l) <= mw) break;
            if(!I->isFalse(l)) continue;
            // without l (and the lighter false ones) the group reaches w(l) at most
            if(slack && weightOf(l) - mw < slack->get(derived)){ slack->get(derived) -= weightOf(l) - mw; break; }
            R.push_back(l);
        }
    }
}

// ge (port of create_reason_true_ge): without the derived literal its group can't exceed the second maximum, the literals
// above it are false (ijcai: all the false literals of the group); min/minfly leave out what the slack allows
void AmosumPropagator::explainMaxWithout(const clingo_literal_t derived, const clingo_literal_t second, const AmosumThreadGroup& g) noexcept{
    if(decisionLevel == 0) return;
    std::vector<clingo_literal_t>& R = reasons->get(derived);
    const size_t i = second != SETTINGS::NONE_LITERAL && !prop->fullTrueReason ? g.ordI.at(second) : 0;
    const int64_t ws = weightOf(second);
    for(size_t k = g.ordL.size(); k-- > i;){
        const clingo_literal_t lit = g.ordL[k];
        if(!prop->fullTrueReason && weightOf(lit) <= ws) break;
        if(!I->isFalse(lit) || std::abs(lit) == std::abs(derived)) continue;
        // without lit (and the lighter ones) the group without the derived literal reaches w(lit) at most
        if(slack && weightOf(lit) - ws < slack->get(derived)){ slack->get(derived) -= weightOf(lit) - ws; break; }
        R.push_back(lit);
    }
}

// le_eo (port of create_reason_falses_le): each group can't go below its current value
void AmosumPropagator::explainMinOfGroups(const clingo_literal_t derived, const size_t excluded) noexcept{
    if(decisionLevel == 0) return;
    std::vector<clingo_literal_t>& R = reasons->get(derived);
    for(size_t gi = 0; gi < groups.size(); ++gi){
        if(gi == excluded) continue;
        const AmosumThreadGroup& g = groups[gi];
        if(g.trueLiteral != SETTINGS::NONE_LITERAL){
            // a moved true literal (worth 0: never above the minimum) or ordL emptied at level 0: nothing to explain
            if(g.trueIsMoved || g.ordL.empty()) continue;
            R.push_back(fiore::not_(g.trueLiteral));
            continue;
        }
        if(g.movedNotFalse > 0) continue;               // minimum 0: holds in any assignment
        const int64_t mw = weightOf(g.minUnd());
        for(const clingo_literal_t l: g.ordL){
            if(weightOf(l) > mw) break;
            if(I->isFalse(l)) R.push_back(l);
        }
        if(mw > 0) for(const clingo_literal_t s: *g.movedLiterals) R.push_back(s);   // the minimum is not 0: all moved literals are false
    }
}

// le_amo (port of create_reason_le_amo): the true literals that make up the current sum
void AmosumPropagator::explainTrueOfGroups(const clingo_literal_t derived, const size_t excluded) noexcept{
    if(decisionLevel == 0) return;
    std::vector<clingo_literal_t>& R = reasons->get(derived);
    for(size_t gi = 0; gi < groups.size(); ++gi){
        if(gi == excluded) continue;
        const AmosumThreadGroup& g = groups[gi];
        if(g.trueLiteral == SETTINGS::NONE_LITERAL) continue;
        // a moved true literal (worth 0 in the current sum) or ordL emptied at level 0: nothing to explain
        if(g.trueIsMoved || g.ordL.empty()) continue;
        R.push_back(fiore::not_(g.trueLiteral));
    }
}