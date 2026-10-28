// the search algorithms (sat-unsat, unsat-sat, binary, with every progression) and AmosumClingoAssignment
#include <algorithm>
#include <regex>
#include "unit.h"
#include "helpers.h"

using helpers::search;
using helpers::STRATEGIES;
using helpers::PROGRESSIONS;

namespace {
    // every (strategy, progression) pair; binary does not use the progression: once
    std::vector<std::pair<std::string, std::string>> configurations(){
        std::vector<std::pair<std::string, std::string>> all;
        for(const std::string& s: STRATEGIES)
            for(const std::string& p: (s == "binary" ? std::vector<std::string>{"1"} : PROGRESSIONS)) all.emplace_back(s, p);
        return all;
    }
    std::string label(const std::string& strategy, const std::string& progression){ return strategy + " --progression " + progression; }

    // every configuration finds the optimum, proves it, and prints strictly increasing values ending at it
    void checkOptimum(const std::string& data, int64_t optimum){
        for(const auto& [strategy, progression]: configurations()){
            const helpers::Search s = search(data, strategy, progression);
            const std::string name = label(strategy, progression);
            // the label in both values: a failure says which configuration
            CHECK_EQ(name + " optimum proven: " + unit::show(s.result.isOptimum()), name + " optimum proven: true");
            CHECK_EQ(name + " value: " + std::to_string(s.value), name + " value: " + std::to_string(optimum));
            const std::vector<int64_t> values = helpers::printedValues(s.printed);
            const bool increasing = std::adjacent_find(values.begin(), values.end(), std::greater_equal<int64_t>()) == values.end();
            CHECK_EQ(name + " values increase: " + unit::show(increasing), name + " values increase: true");
            CHECK_EQ(name + " last printed: " + (values.empty() ? std::string("none") : std::to_string(values.back())),
                     name + " last printed: " + std::to_string(optimum));
        }
    }
}

TEST(search_optimum_far_from_the_maximum){
    checkOptimum("max_far", 17);            // maximum possible value 55: the exponential bounds overshoot and come back
}

TEST(search_optimum_with_negative_weights){
    checkOptimum("max_negative", -1);       // every value is below the bound 0: sat-unsat starts from the minimum
}

TEST(search_level0_conflict_is_volatile){
    // x(5) is false before search: a bound above 10 is violated already at init. The conflict must hold for that
    // solve call only (fiore's volatile init clauses), or unsat-sat would answer UNSAT
    checkOptimum("max_level0", 10);
}

TEST(search_unsat_amomaximize){
    for(const auto& [strategy, progression]: configurations()){
        const helpers::Search s = search("max_unsat", strategy, progression);
        CHECK_EQ(label(strategy, progression) + ": " + unit::show(s.result.isUnsat()), label(strategy, progression) + ": true");
    }
}

TEST(search_clause_type_of_the_amomaximize_propagator){
    // learnt only where the bound never decreases (sat-unsat with step 1), volatile otherwise
    CHECK(!search("max_far", "sat-unsat", "1").volatileClauses);
    CHECK(search("max_far", "sat-unsat", "2^n").volatileClauses);
    CHECK(search("max_far", "unsat-sat", "1").volatileClauses);
    CHECK(search("max_far", "binary").volatileClauses);
}

TEST(search_without_amomaximize_is_one_solve_call){
    for(const std::string& strategy: STRATEGIES){
        const helpers::Search sat = search("ge_amo", strategy);
        CHECK(sat.result.isSat());
        CHECK(!sat.result.isOptimum());                     // nothing to optimize: SAT, not optimum
        CHECK(sat.answerSet.find("'c'") != std::string::npos);   // >= 4 needs c (3) plus a or b
        CHECK(sat.printed.empty());                         // no "Amomaximize Cost" line
        CHECK(search("ge_amo_unsat", strategy).result.isUnsat());
    }
}

TEST(assignment_hides_auxiliary_atoms_but_keeps_them_true){
    auto solver = helpers::solverFor("le_eo");
    helpers::CaptureOutput quiet;
    auto [answerset, result] = solver->solve();
    REQUIRE(result.isSat());
    // shown: the program's own atoms only
    CHECK(answerset->outputString.find("'x(1)'") != std::string::npos);
    CHECK(!std::regex_search(answerset->outputString, std::regex("__(element|group|aux|lb|ub|tuple|same)__")));
    // but in the assignment the auxiliary atoms keep their value (__ub__ and __aux__ are facts)
    int auxiliaryTrue = 0;
    for(const fiore::Literal* l: *solver->program->restLiterals)
        if(l->name.rfind("__ub__", 0) == 0 || l->name.rfind("__aux__", 0) == 0) auxiliaryTrue += answerset->isTrue(l);
    CHECK_EQ(auxiliaryTrue, 2);
    fiore::AnswersetsPool::getInstance()->deleteAnswerset(answerset);
}

TEST(assignment_amosum_cost_is_the_value_of_the_model){
    // max_far: the value of a model is the sum of the I of its x(I)
    auto solver = helpers::solverFor("max_far");
    helpers::CaptureOutput quiet;
    auto [answerset, result] = solver->solve();
    REQUIRE(result.isSat());
    int64_t sum = 0;
    const std::regex atom("'x\\((\\d+)\\)'");
    for(auto it = std::sregex_iterator(answerset->outputString.begin(), answerset->outputString.end(), atom); it != std::sregex_iterator(); ++it)
        sum += std::stoll((*it)[1]);
    CHECK_EQ(static_cast<AmosumClingoAssignment*>(answerset)->amosumCost, sum);
    fiore::AnswersetsPool::getInstance()->deleteAnswerset(answerset);
}
