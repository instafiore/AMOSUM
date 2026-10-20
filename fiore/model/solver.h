#pragma once
#include "../utils/utility.h"
#include "propagatorPBAbstraction.h"
#include "propagator.h"
#include "assigment.h"
#include "core.h"


namespace fiore {

enum SolverDecisioner{
    Decision0
};

struct PropagatorGetRestLiterals;
struct PropagatorGetDerivedFromAss;





struct ClingoSolver {

    std::unique_ptr<Program> program;
    std::unique_ptr<ClingoPropagatorInitializer> initializer;
    std::vector<std::unique_ptr<ClingoPropagator>> propagators;
    ClingoPropagator* optimizer;
    ClingoAssignment* answerset;
    clingo::ClingoResult res;
    std::vector<clingo_literal_t> assumptions;    
    std::unordered_set<clingo_literal_t> assumptionsSet;    
    

    void registerPropagator(ClingoPropagator* prop, std::string name = "");

    ClingoSolver(std::vector<std::string> ASPInputs) noexcept;

    inline PBConstraint* createOptimizer(int64_t upperBound = CONSTANTS::INFPlus64, Vector<const Literal*>* &&softLiterals = nullptr, UMNWeightFunction* weightFunction = nullptr)noexcept{
        PBConstraint* pb = program->addPBConstraint(upperBound, std::move(softLiterals), weightFunction);
        PropagatorPBConstraint* pbProp = new PropagatorPBConstraint("PbProp", this, pb);
        registerPropagator(pbProp);
        optimizer = pbProp;
        return pb;
    }

    inline void setPolarity0(size_t priority = CONSTANTS::INFPlus32) noexcept { if(!changedPolTargets) return; changedPolTargets = false; this->program->setPolaritiesLocalSearch(nullptr, priority, nullptr); }
    inline void setPolarityBasedOnAssignment(const Assignment* ass, size_t priority = CONSTANTS::INFPlus32) noexcept { changedPolTargets = true; this->program->setPolaritiesLocalSearch(ass, priority, nullptr); }
    void setTORC(const Assignment* a, size_t priority = CONSTANTS::INFPlus32) noexcept;
    void setRest(const Assignment* a, size_t priority = CONSTANTS::INFPlus32) noexcept;
    std::pair<ClingoAssignment*, const clingo::ClingoResult> solve(const std::vector<clingo_literal_t>& assumptions = {}) noexcept;
    bool atLeast2Models(const std::vector<clingo_literal_t>* assumptions);
    std::pair<std::vector<clingo_literal_t>, std::unordered_map<clingo_literal_t, std::vector<clingo_literal_t>>> removeFromIToLastAssumptionAndReturn(const size_t i) noexcept;


    void setConflictLimit(std::string max_conflicts);
    inline void setNoConflictLimits() noexcept { this->setConfiguration("solve/solve_limit","umax"); }

    void setConfiguration(std::string path, std::string value) {
        clingo_configuration_t *configuration;
        clingo_control_configuration(program->ctl, &configuration);
        clingo::setValueConfigurationClingo(configuration, path, value);
    }


    inline double getStatistic(std::string path) const noexcept { 
        const clingo_statistics_t *stats;
        handleError(clingo_control_statistics(this->program->ctl, &stats)); 
        return clingo::getValueStatisticClingo(stats, path); 
    }
    inline std::string lastCallAnswerSetToString() noexcept { return this->answerset ? this->answerset->toString() : std::string("No answer set given"); }
    bool isDerivedFromCurrentAssumptions(const clingo_literal_t& plit) noexcept;
    const std::vector<clingo_literal_t>&  derivedLiteralsFromAssumptionIndex(const size_t& i) noexcept;
    const std::vector<clingo_literal_t>&  derivedLiteralsFromAssumption(const clingo_literal_t& assumption) noexcept;
    int64_t getCostFromFacts(const UMNWeightFunction* weightFunction = nullptr) noexcept;

    std::vector<const Literal*> literalIdsToLiterals(const std::vector<clingo_literal_t>& literalIds) noexcept{
        std::vector<const Literal*> res;
        MapLiteralId2Literal* mapLitId2Lit = this->program->mapLiteralId2Literal.get();
        for(size_t i = 0; i < literalIds.size(); ++i) res.push_back(mapLitId2Lit->get(literalIds[i]));
        return res;
    }
    const Literal* literalIdToLiteral(clingo_literal_t literalId) noexcept{
        return literalIdsToLiterals(std::vector<clingo_literal_t>{literalId})[0];
    }

    void enableRestDetection();
    void disableRestDetection();

    void enableGetDerivedFromAssDetection();
    void disableGetDerivedFromAssDetection();
    
    void addAssumption(const clingo_literal_t &l) noexcept;
    void addAssumptionAndSet(const clingo_literal_t &l, std::vector<clingo_literal_t>&& derivedFromL) noexcept;
    void clearAssumptions() noexcept;
    inline bool isInAssumptions(const clingo_literal_t& l) const noexcept { return assumptionsSet.find(l) != assumptionsSet.end(); }
    void removeFromIToLastAssumption(const size_t& i) noexcept;
    inline void removeLastKAssumptions(const size_t& k) noexcept { size_t n = assumptions.size(); size_t i = std::min(n, k); removeFromIToLastAssumption(n-i); }
    void removeLastAssumption() noexcept;
    inline const Core* getBlamedAssumptions() const noexcept{ return this->res.isSat() ? nullptr : &blamedAssumptions; }
    inline const std::vector<Core*>& getUnsatCores() const noexcept{ return unsatCores; }

    ~ClingoSolver();

    #ifdef __debug__
        inline void restartWallTimePropagators() noexcept { for(std::unique_ptr<ClingoPropagator>& p: propagators) p->resetTimes(); }
    #endif

    size_t countSolve = 0;
    int64_t getCoreLowerBound() const noexcept;
    void addForcedToLowerBound(const clingo_literal_t& l) noexcept;
    inline void setCollectUnsatCores(const bool set) noexcept { collectUnsatCores = set; }
    inline int64_t getLowerBound() const noexcept { return  program->preprocessedCost + coreLowerBound.value(); }
    inline bool isLowerBoundAtLeastPPercentageOfAnswerset(Assignment* ans, const float p) const noexcept { return (double)this->getLowerBound() / (double)std::max<int64_t>(1, ans->cost()) >= p; }

private:
   
    CoreLowerBound coreLowerBound;
    PropagatorGetRestLiterals* getterRestLiterals;
    PropagatorGetDerivedFromAss* getterDerivedFromAss;
    bool changedPolTargets;
    void updateStatistics() const noexcept;   // copies the clingo statistics of the last call into Statistics
    bool collectUnsatCores;
    Core blamedAssumptions;
    std::vector<Core*> unsatCores;
    #ifdef __debug__
    double totalTimeSolve = 0;
    double totalTimeSolveReal = 0;
    double firstCallTime = 0;
    #endif


};

} // namespace fiore
