#pragma once
// A minimal unit-test framework, without dependencies.
//
//   TEST(name){ ... }            defines a test; it registers itself before main runs
//   CHECK(cond)                  a failure is reported and the test goes on
//   CHECK_EQ(actual, expected)   as CHECK, printing both values when they differ
//   CHECK_THROWS(expr, Type)     expr must throw a Type
//   REQUIRE(cond)                as CHECK, but a failure ends the test (when the rest would not make sense)
//
// unit::runAll runs every registered test (or those whose name contains a filter) and returns the number of
// failed tests: 0 means success, which makes the process exit code usable by make and CI.
#include <cstdio>
#include <exception>
#include <sstream>
#include <string>
#include <vector>

namespace unit {

    struct TestCase{
        const char* name;
        void (*body)();
        const char* file;
        int line;
    };

    // a function-local static: built on first use, so registration works whatever the order in which the
    // translation units are initialized
    inline std::vector<TestCase>& registry(){ static std::vector<TestCase> tests; return tests; }
    inline int& failedChecks(){ static int n = 0; return n; }

    struct Registrar{
        Registrar(const char* name, void (*body)(), const char* file, int line){ registry().push_back({name, body, file, line}); }
    };

    template<typename T>
    std::string show(const T& value){ std::ostringstream out; out << value; return out.str(); }
    inline std::string show(const std::string& value){ return "\"" + value + "\""; }
    inline std::string show(const char* value){ return "\"" + std::string(value) + "\""; }
    inline std::string show(bool value){ return value ? "true" : "false"; }
    template<typename T>
    std::string show(const std::vector<T>& values){
        std::string out = "{";
        for(size_t i = 0; i < values.size(); ++i) out += (i ? ", " : "") + show(values[i]);
        return out + "}";
    }

    inline void fail(const char* file, int line, const std::string& what){
        std::printf("    FAIL %s:%d: %s\n", file, line, what.c_str());
        ++failedChecks();
    }

    // runs the tests whose name contains filter ("" = all); returns the number of failed tests
    inline int runAll(const std::string& filter = ""){
        int failedTests = 0, run = 0;
        for(const TestCase& t: registry()){
            if(!filter.empty() && std::string(t.name).find(filter) == std::string::npos) continue;
            ++run;
            const int before = failedChecks();
            try{
                t.body();
            }catch(const std::exception& e){
                fail(t.file, t.line, std::string("unexpected exception: ") + e.what());
            }catch(...){
                fail(t.file, t.line, "unexpected exception");
            }
            const bool ok = failedChecks() == before;
            failedTests += !ok;
            std::printf("%s %s\n", ok ? "ok  " : "FAIL", t.name);
        }
        std::printf("\n%s: %d/%d tests passed\n", failedTests == 0 ? "PASSED" : "FAILED", run - failedTests, run);
        return failedTests;
    }
}

#define UNIT_CONCAT_(a, b) a##b
#define UNIT_CONCAT(a, b) UNIT_CONCAT_(a, b)

// static: each test is local to its file; the registrar is a global object whose constructor runs before main
#define TEST(name) \
    static void name(); \
    static const unit::Registrar UNIT_CONCAT(name, _registrar)(#name, &name, __FILE__, __LINE__); \
    static void name()

#define CHECK(cond) \
    do{ if(!(cond)) unit::fail(__FILE__, __LINE__, "CHECK(" #cond ")"); }while(0)

#define CHECK_EQ(actual, expected) \
    do{ \
        const auto& unit_a = (actual); const auto& unit_e = (expected); \
        if(!(unit_a == unit_e)) \
            unit::fail(__FILE__, __LINE__, "CHECK_EQ(" #actual ", " #expected "): " + unit::show(unit_a) + " != " + unit::show(unit_e)); \
    }while(0)

#define CHECK_THROWS(expr, Type) \
    do{ \
        bool unit_thrown = false; \
        try{ (void)(expr); }catch(const Type&){ unit_thrown = true; }catch(...){} \
        if(!unit_thrown) unit::fail(__FILE__, __LINE__, "CHECK_THROWS(" #expr ", " #Type "): nothing thrown"); \
    }while(0)

#define REQUIRE(cond) \
    do{ if(!(cond)){ unit::fail(__FILE__, __LINE__, "REQUIRE(" #cond ")"); return; } }while(0)
