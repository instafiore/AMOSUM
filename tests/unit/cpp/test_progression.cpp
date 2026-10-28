// Progression: the --progression expression in the step n (parser, evaluation, limits, errors)
#include <stdexcept>
#include "unit.h"
#include "amosum/amoclingo/cpp/amoclingo_algorithms.h"

static int64_t at(const std::string& expression, size_t n){ return Progression(expression).at(n); }

TEST(progression_constants){
    for(size_t n = 0; n < 5; ++n){
        CHECK_EQ(at("1", n), 1);
        CHECK_EQ(at("2", n), 2);
    }
}

TEST(progression_powers_and_polynomials){
    CHECK_EQ(at("2^n", 0), 1);
    CHECK_EQ(at("2^n", 1), 2);
    CHECK_EQ(at("2^n", 5), 32);
    CHECK_EQ(at("n^2", 3), 9);
    CHECK_EQ(at("3*n+1", 4), 13);
    CHECK_EQ(at("(n+1)^2", 2), 9);
}

TEST(progression_values_below_one_count_as_one){
    CHECK_EQ(at("n^2", 0), 1);          // 0
    CHECK_EQ(at("-5", 0), 1);
    CHECK_EQ(at("2^-1", 0), 1);         // a negative exponent gives 0
    CHECK_EQ(at("n-10", 3), 1);
}

TEST(progression_precedence){
    CHECK_EQ(at("2+3*n", 2), 8);        // * before +
    CHECK_EQ(at("(2+3)*n", 2), 10);
    CHECK_EQ(at("2*n^2", 3), 18);       // ^ before *
    CHECK_EQ(at("10+-2^2", 0), 6);      // -(2^2), not (-2)^2
}

TEST(progression_associativity){
    CHECK_EQ(at("10-3-2", 0), 5);       // + - * from the left: (10-3)-2
    CHECK_EQ(at("2^n^2", 3), 512);      // ^ from the right: 2^(3^2), not (2^3)^2 = 64
}

TEST(progression_spaces){
    CHECK_EQ(at(" 2 ^ n ", 3), 8);
}

TEST(progression_capped_at_2_to_62){
    const int64_t cap = int64_t(1) << 62;
    CHECK_EQ(at("2^n", 100), cap);
    CHECK_EQ(at("n^100", 10), cap);
    CHECK_EQ(at("99999999999999999999", 0), cap);
    CHECK_EQ(at("2^n*2^n", 40), cap);   // the product would overflow 64 bits
}

TEST(progression_is_linear_only_for_constant_one){
    CHECK(Progression("1").isLinear());
    CHECK(Progression("2-1").isLinear());
    CHECK(Progression("-5").isLinear());     // counts as 1
    CHECK(!Progression("2").isLinear());
    CHECK(!Progression("2^n").isLinear());
    CHECK(!Progression("n^0").isLinear());   // 1 in value, but it uses n: treated as non linear (safe side)
}

TEST(progression_keeps_its_text){
    CHECK_EQ(Progression("2^n").expression, std::string("2^n"));
}

TEST(progression_malformed_expressions_throw){
    for(const std::string bad: {"", " ", "2^", "(n+1", "2 3", "x", "*2", "n)", "2**n", "+"})
        CHECK_THROWS(Progression(bad), std::invalid_argument);
}

TEST(progression_error_message_has_the_position){
    try{
        Progression p("2^");
        unit::fail(__FILE__, __LINE__, "no exception");
    }catch(const std::invalid_argument& e){
        const std::string message = e.what();
        CHECK(message.find("unexpected end") != std::string::npos);
        CHECK(message.find("position 3") != std::string::npos);
    }
}
