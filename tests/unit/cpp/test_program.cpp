// AmosumProgram: the grounding of the rewritten program into AmosumConstraints and their groups
// (aggregate type, bound, groups and weights, the normalization of shared tuples and equal weights)
#include <algorithm>
#include <set>
#include "unit.h"
#include "helpers.h"

using helpers::programOf;
using helpers::solverFor;

namespace {
    std::vector<const ProgramGroup*> nonAuxGroups(const AmosumConstraint* c){
        std::vector<const ProgramGroup*> groups;
        for(const ProgramGroup& g: c->groups) if(!g.aux) groups.push_back(&g);
        return groups;
    }
    std::multiset<int64_t> weights(const ProgramGroup* g){
        std::multiset<int64_t> w;
        for(const auto& [literal, weight]: g->members) w.insert(weight);
        return w;
    }
    const ProgramGroup* groupWithId(const AmosumConstraint* c, const std::string& prefix){
        for(const ProgramGroup& g: c->groups) if(g.id.rfind(prefix, 0) == 0) return &g;
        return nullptr;
    }
    bool startsWith(const std::string& s, const std::string& prefix){ return s.rfind(prefix, 0) == 0; }
}

TEST(program_ge_amo_constraint){
    auto solver = solverFor("ge_amo");
    AmosumProgram* p = programOf(*solver);
    REQUIRE(p->amosumConstraints.size() == 1);
    const AmosumConstraint* c = p->amosumConstraints[0];
    CHECK_EQ(c->type, std::string("ge_amo"));
    CHECK(c->ge);
    CHECK_EQ(c->bound, 4);
    CHECK(p->amomaximizeConstraint == nullptr);
    // g1 = {a:1, b:2}, g2 = {c:3}, plus the aux group of the "-" element
    const auto groups = nonAuxGroups(c);
    REQUIRE(groups.size() == 2);
    std::set<std::multiset<int64_t>> all = {weights(groups[0]), weights(groups[1])};
    CHECK((all == std::set<std::multiset<int64_t>>{{1, 2}, {3}}));
    CHECK_EQ(c->groups.size() - groups.size(), (size_t)1);
}

TEST(program_le_eo_constraint){
    auto solver = solverFor("le_eo");
    AmosumProgram* p = programOf(*solver);
    REQUIRE(p->amosumConstraints.size() == 1);
    const AmosumConstraint* c = p->amosumConstraints[0];
    CHECK_EQ(c->type, std::string("le_eo"));
    CHECK(!c->ge);
    CHECK_EQ(c->bound, 1);
    const auto groups = nonAuxGroups(c);
    REQUIRE(groups.size() == 1);
    CHECK((weights(groups[0]) == std::multiset<int64_t>{1, 2, 3}));
}

TEST(program_shared_tuple_counts_once){
    // the tuple 1 is in g1 and g2: its weight moves to a __tuple__ group (an OR of the two element atoms),
    // the element atoms stay in g1 and g2 with weight 0
    auto solver = solverFor("shared_tuple");
    const AmosumConstraint* c = programOf(*solver)->amosumConstraints.at(0);
    const ProgramGroup* tuple = groupWithId(c, "__tuple__");
    REQUIRE(tuple != nullptr);
    REQUIRE(tuple->members.size() == 1);                  // AMO: no negated member (only EO adds one)
    CHECK_EQ(tuple->members[0].second, 1);
    CHECK(startsWith(tuple->members[0].first->name, "__tuple__"));
    for(const ProgramGroup* g: nonAuxGroups(c)){
        if(g == tuple) continue;
        CHECK((weights(g) == std::multiset<int64_t>{0}));
    }
}

TEST(program_equal_weights_in_a_group_are_merged){
    // (2,a) and (2,b) in the same group g: one member, the __same__ OR of the two, with weight 2
    auto solver = solverFor("same_weight");
    const AmosumConstraint* c = programOf(*solver)->amosumConstraints.at(0);
    const ProgramGroup* g = groupWithId(c, "g");
    REQUIRE(g != nullptr);
    REQUIRE(g->members.size() == 1);
    CHECK_EQ(g->members[0].second, 2);
    CHECK(startsWith(g->members[0].first->name, "__same__"));
}

TEST(program_amomaximize){
    auto solver = solverFor("max_far");
    AmosumProgram* p = programOf(*solver);
    REQUIRE(p->amomaximizeConstraint != nullptr);
    CHECK_EQ(p->amomaximizeConstraint->type, std::string("amomaximize"));
    CHECK(!p->isOptimizationProblem);                       // set by #minimize only
    CHECK(solver->maximizer != nullptr);                    // its propagator
    CHECK(solver->maximizer->constraint == p->amomaximizeConstraint);
}

TEST(program_possible_values_of_amomaximize){
    auto far = solverFor("max_far");                        // x(1..10), one group each
    CHECK_EQ(amosumSearch::maxPossibleValue(programOf(*far)->amomaximizeConstraint), 55);
    CHECK_EQ(amosumSearch::minPossibleValue(programOf(*far)->amomaximizeConstraint), 0);
    auto negative = solverFor("max_negative");              // one group, weights -3..-1
    CHECK_EQ(amosumSearch::maxPossibleValue(programOf(*negative)->amomaximizeConstraint), 0);
    CHECK_EQ(amosumSearch::minPossibleValue(programOf(*negative)->amomaximizeConstraint), -3);
}
