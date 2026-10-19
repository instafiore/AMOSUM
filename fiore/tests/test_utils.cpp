// Tests for fiore/utils. Built without any Clingo include path or library (see Makefile):
// it also checks that the utilities do not depend on Clingo.
//
// usage: test_utils <output dir for log and stats files>

#include "fiore/utils/utility.h"
#include "fiore/utils/logger.h"
#include "fiore/utils/stats.h"
#include <cstdio>
#include <sstream>

using namespace fiore;

static int failures = 0;
#define CHECK(cond) do { if(!(cond)){ std::printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond); ++failures; } } while(0)

template<typename T>
static std::string str(const T& x){ std::ostringstream oss; oss<<x; return oss.str(); }

int main(int argc, char** argv){
    const std::string outDir = argc > 1 ? argv[1] : ".";
    std::string statsArg = "--" + CONSTANTS::STATSFILEKey + "=" + outDir + "/test_utils.stats";
    std::string logArg = "--" + CONSTANTS::LogFileNameKey + "=" + outDir + "/test_utils.log";
    std::string appArg = "--my-option=7";
    std::vector<char*> args = {argv[0], statsArg.data(), logArg.data(), appArg.data()};

    // ArgParse: library options plus an application option registered at init
    startWallTime();
    ArgParse::init((int)args.size(), args.data(),
                   {Argument::Builder("my-option").isWithValue().withDesc("an application option").withDefaultValue("1").withRegex(PositiveInteger()).build()});
    CHECK(ArgParse::getInstance()->get("my-option") == "7");
    CHECK(ArgParse::getInstance()->get(CONSTANTS::seedKey) == CONSTANTS::DefaultSeed);
    #ifdef __debug__
        Logger::init(ArgParse::getInstance()->get(CONSTANTS::LogFileNameKey), false, LogLevel::DEBUG);
        LOG(LogLevel::INFO, "test_utils started");
    #endif
    Statistics::initInstance(ArgParse::getInstance()->get(CONSTANTS::STATSFILEKey));

    // containers
    const Literal* a = new Literal(1, "a");
    const Literal* b = new Literal(2, "b");
    Vector<const Literal*> v(0);
    v.add(a); v.add(b);
    CHECK(v.size() == 2 && v.contains(b));
    v.reverse();
    CHECK(v.get(0) == b && v.index(a) == 1);
    CHECK(str(v) == "['(b, 2)', '(a, 1)']");

    PerfectIterableSet<const Literal*> s(10);
    s.add(a); s.add(b); s.add(a);
    CHECK(s.size() == 2);
    s.remove(a);
    CHECK(s.size() == 1 && !s.contains(a) && s.contains(b));

    PerfectNHash<int32_t, int> h(10, 0);
    h.set(-3, 5); h.set(3, 4);
    CHECK(h.get(-3) == 5 && h.get(3) == 4 && h.get(2) == 0);

    CHECK(join(",", std::vector<int>{1, 2, 3}) == "1,2,3");

    // Statistics: generic counters and summaries, written as JSON
    Statistics* stats = Statistics::getInstance();
    stats->increaseGenericStat("calls", 1);
    stats->increaseGenericStat("calls", 2);
    stats->updateGenericStat("name", "test");
    const std::string json = stats->toJson();
    CHECK(json.find("\"calls\"") != std::string::npos && json.find("\"name\": \"test\"") != std::string::npos);

    delete a; delete b;
    cleanup();
    std::printf("%s: test_utils, %d failed checks\n", failures == 0 ? "PASSED" : "FAILED", failures);
    return failures == 0 ? 0 : 1;
}
