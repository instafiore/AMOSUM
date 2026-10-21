// Tests for fiore/model: Program, ClingoSolver and the pseudo-Boolean propagator
// (createOptimizer -> PropagatorPBConstraint) used in a linear SAT-UNSAT loop.
// The expected costs were computed with `clingo --opt-mode=opt`.
//
// usage: test_solver <output dir for log and stats files>

#include "fiore/utils/utility.h"
#include "fiore/utils/logger.h"
#include "fiore/utils/stats.h"
#include "fiore/clingo/utilityClingo.h"
#include "fiore/model/solver.h"
#include "fiore/model/program.h"
#include "fiore/model/assigment.h"
#include <cstdio>

using namespace fiore;

static int failures = 0;
#define CHECK(cond) do { if(!(cond)){ std::printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond); ++failures; } } while(0)

struct Case { std::string file; bool sat; bool optimization; int64_t optimum; };

static const std::vector<Case> CASES = {
    {"tests/data/choice.asp",       true,  true,  1},
    {"tests/data/cover.asp",        true,  true,  7},
    {"tests/data/vertex_cover.asp", true,  true,  89},
    {"tests/data/nonopt.asp",       true,  false, 0},
    {"tests/data/unsat.asp",        false, false, 0},
};

// linear SAT-UNSAT loop on the PB constraint (as the LSU algorithm): the optimum, or -1 if unsatisfiable
static int64_t lsu(ClingoSolver& solver, bool& optimization){
    PBConstraint* pb = solver.createOptimizer(CONSTANTS::INFPlus64);
    solver.setNoConflictLimits();
    solver.clearAssumptions();
    solver.disableGetDerivedFromAssDetection();
    solver.disableRestDetection();
    solver.setCollectUnsatCores(false);
    clingo::setVSIDSHeuristic(solver.program->ctl);

    optimization = solver.program->isOptimizationProblem;
    int64_t best = -1;
    while(true){
        std::pair<ClingoAssignment*, const clingo::ClingoResult> r = solver.solve();
        if(!r.second.isSat()) break;
        const int64_t cost = r.first->cost();
        AnswersetsPool::getInstance()->deleteAnswerset(r.first);
        if(cost > pb->bound){ std::printf("FAIL cost %lld above the bound %lld\n", (long long)cost, (long long)pb->bound); ++failures; break; }
        best = optimization ? cost : 0;
        if(!optimization) break;
        pb->updateBound(cost - 1);   // ask for a strictly better answer set
    }
    return best;
}

int main(int argc, char** argv){
    const std::string outDir = argc > 1 ? argv[1] : ".";
    std::string statsArg = "--" + CONSTANTS::STATSFILEKey + "=" + outDir + "/test_solver.stats";
    std::string logArg = "--" + CONSTANTS::LogFileNameKey + "=" + outDir + "/test_solver.log";
    std::vector<char*> args = {argv[0], statsArg.data(), logArg.data()};

    startWallTime();
    ArgParse::init((int)args.size(), args.data());
    #ifdef __debug__
        Logger::init(ArgParse::getInstance()->get(CONSTANTS::LogFileNameKey), false,
                     Logger::stringToLevel(ArgParse::getInstance()->get(CONSTANTS::logKey)));
    #endif
    Statistics::initInstance(ArgParse::getInstance()->get(CONSTANTS::STATSFILEKey));

    // default solver (rest and derived-from-assumption detection on): an unsatisfiable set of
    // assumptions gives a core of them; the choice program needs a or b
    {
        ClingoSolver solver;
        solver.init({"tests/data/choice.asp"});
        std::vector<clingo_literal_t> assumptions;
        for(const Literal* l: *solver.program->softLiterals)
            if(l->name == "a" || l->name == "b") assumptions.push_back(not_(l->id));
        CHECK(assumptions.size() == 2);
        std::pair<ClingoAssignment*, const clingo::ClingoResult> r = solver.solve(assumptions);
        CHECK(r.second.isUnsat());
        const Core* core = solver.getBlamedAssumptions();
        CHECK(core != nullptr && core->size() > 0);
        std::printf("ok   tests/data/choice.asp           unsat under assumptions, core size %zu\n", core ? core->size() : 0);
        std::pair<ClingoAssignment*, const clingo::ClingoResult> free = solver.solve();
        CHECK(free.second.isSat());
        AnswersetsPool::getInstance()->deleteAnswerset(free.first);
    }

    for(const Case& c: CASES){
        ClingoSolver solver;
        solver.init({c.file});
        bool optimization = false;
        const int64_t got = lsu(solver, optimization);
        const bool sat = got >= 0;
        const bool ok = sat == c.sat && (!sat || optimization == c.optimization) && (!c.optimization || got == c.optimum);
        std::printf("%-4s %-30s sat=%d optimization=%d cost=%lld (expected sat=%d optimum=%lld)\n",
                    ok ? "ok" : "FAIL", c.file.c_str(), sat, optimization, (long long)got, c.sat, (long long)c.optimum);
        failures += !ok;
    }

    cleanup();
    std::printf("%s: test_solver, %d failed\n", failures == 0 ? "PASSED" : "FAILED", failures);
    return failures == 0 ? 0 : 1;
}
