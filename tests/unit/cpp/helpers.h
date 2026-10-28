#pragma once
// helpers shared by the tests that ground and solve programs (data/*.lp, written by the parser from data/*.asp)
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <vector>
#include "amosum/amoclingo/cpp/solver.h"
#include "amosum/core/cpp/search/algorithms.h"
#include "fiore/model/assigment.h"

#ifndef DATA_DIR
#error "DATA_DIR (the tests/unit/data folder) must be defined by the Makefile"
#endif

namespace helpers {

    inline std::string dataFile(const std::string& name){ return std::string(DATA_DIR) + "/" + name + ".lp"; }

    // what is printed on std::cout while it lives (the algorithms print every model they find)
    class CaptureOutput{
    public:
        CaptureOutput(): previous(std::cout.rdbuf(captured.rdbuf())){}
        ~CaptureOutput(){ std::cout.rdbuf(previous); }
        std::string text() const { return captured.str(); }
    private:
        std::ostringstream captured;
        std::streambuf* previous;
    };

    // a solver on data/<name>.lp, initialized (grounded, propagators registered)
    inline std::unique_ptr<AmosumClingoSolver> solverFor(const std::string& name){
        auto solver = std::make_unique<AmosumClingoSolver>("nomin", "false");
        solver->init({dataFile(name)});
        return solver;
    }

    inline AmosumProgram* programOf(AmosumClingoSolver& solver){ return static_cast<AmosumProgram*>(solver.program.get()); }

    // the result of one search with an --opt-strategy and a --progression
    struct Search{
        fiore::clingo::ClingoResult result;
        int64_t value = 0;                  // amosumCost of the best known answer set (if SAT)
        std::string answerSet;              // its printed atoms
        std::string printed;                // everything the algorithm printed
        bool volatileClauses = false;       // clause type of the #amomaximize propagator after the search
    };

    inline const std::vector<std::string> STRATEGIES = {"sat-unsat", "unsat-sat", "binary"};
    inline const std::vector<std::string> PROGRESSIONS = {"1", "2", "2^n", "n^2"};

    inline Search search(const std::string& name, const std::string& strategy, const std::string& progressionText = "1"){
        Search s;
        CaptureOutput capture;
        auto solver = solverFor(name);
        const Progression progression(progressionText);
        AmosumClingoBackend backend(*solver);
        if(strategy == "sat-unsat") s.result = satUnsat(backend, progression);
        else if(strategy == "unsat-sat") s.result = unsatSat(backend, progression);
        else s.result = binarySearch(backend);
        if(s.result.isSat()){
            // the best known answer set is in the pool: read it, then delete it while its program still exists
            fiore::AnswersetsPool* pool = fiore::AnswersetsPool::getInstance();
            auto* best = static_cast<AmosumClingoAssignment*>(pool->getBestKnownAnswerSet());
            s.value = best->amosumCost;
            s.answerSet = best->outputString;
            pool->deleteAnswerset(best);
        }
        s.volatileClauses = solver->maximizer != nullptr && solver->maximizer->isVolatileClauseType();
        s.printed = capture.text();
        return s;
    }

    // the values of the "Amomaximize Cost: V" lines, in order
    inline std::vector<int64_t> printedValues(const std::string& printed){
        std::vector<int64_t> values;
        std::istringstream lines(printed);
        const std::string prefix = "Amomaximize Cost: ";
        for(std::string line; std::getline(lines, line);)
            if(line.rfind(prefix, 0) == 0) values.push_back(std::stoll(line.substr(prefix.size())));
        return values;
    }
}
