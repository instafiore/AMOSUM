// amosum/utility.h: split, not_, from_symbol_to_string
#include <clingo.h>
#include "unit.h"
#include "amosum/core/cpp/utility.h"

TEST(split_keeps_empty_fields){
    CHECK_EQ(split("a&b&&c", '&'), (std::vector<std::string>{"a", "b", "", "c"}));
    CHECK_EQ(split("one", '&'), (std::vector<std::string>{"one"}));
    CHECK(split("", '&').empty());
}

TEST(not_negates_and_keeps_none){
    CHECK_EQ(not_(5), -5);
    CHECK_EQ(not_(-5), 5);
    CHECK_EQ(not_(SETTINGS::NONE), SETTINGS::NONE);
}

TEST(from_symbol_to_string_prints_clingo_terms){
    clingo_symbol_t symbol;
    REQUIRE(clingo_parse_term("f(a,\"x\",(1,2))", nullptr, nullptr, 0, &symbol));
    CHECK_EQ(from_symbol_to_string(symbol), std::string("f(a,\"x\",(1,2))"));
    clingo_symbol_create_number(-3, &symbol);
    CHECK_EQ(from_symbol_to_string(symbol), std::string("-3"));
}
