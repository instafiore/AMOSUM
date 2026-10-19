#pragma once
#include "../utils/utility.h"
#include "propagator.h"
#include "solver.h"



namespace fiore {

bool initImpPropagatorGetDerivedFromAss(clingo_propagate_init_t *init, ClingoPropagator* prop);
bool propagateImpGetDerivedFromAss(clingo_propagate_control_t *control, clingo_literal_t const *changes, size_t size, ClingoPropagator* propagator);

struct PropagatorGetDerivedFromAss: public ClingoPropagator{

    PropagatorGetDerivedFromAss(std::string name, ClingoSolver* solver, Vector<const Literal*>* targets = nullptr);
    
    inline const Vector<const Literal*>& softLiterals() { return targets != nullptr ? *targets : *solver->program->softLiterals.get(); }
    inline const Vector<const Literal*>& programLiterals() override { return softLiterals(); }
    
    friend bool initImpPropagatorGetDerivedFromAss(clingo_propagate_init_t *init, ClingoPropagator* prop);
    friend bool propagateImpGetDerivedFromAss(clingo_propagate_control_t *control, clingo_literal_t const *changes, size_t size, ClingoPropagator* propagator);

    void addAssumption(const clingo_literal_t& l);
    void addAssumptionAndSet(const clingo_literal_t& l, const std::vector<clingo_literal_t>&& derivedFromL);
    void clearAssumptions();
    // inline bool derivedFromAssumptions(const clingo_literal_t& plit) noexcept{ return targetsDerivedFromAssumptions.find(plit) != targetsDerivedFromAssumptions.end(); }
    inline bool isDerivedFromAssumptions(const clingo_literal_t& plit) noexcept{ return targetsDerivedFromAssumptions->contains(plit); }
    const std::vector<clingo_literal_t>& derivedFromAssumption(const clingo_literal_t& plit) noexcept;
    void removeFromIToLastAssumption(const size_t i) noexcept;
    inline void removeLastAssumption() noexcept { removeFromIToLastAssumption(assumptions.size()-1); }
private:
    void removeAssumption(const clingo_literal_t& l);
    void setDerivedFromAssumption(const clingo_literal_t& plit, std::vector<clingo_literal_t>&& derivedLiterals) noexcept;
    Vector<const Literal*>* targets;
    // unordered_set<clingo_literal_t> targetsDerivedFromAssumptions;
    std::unique_ptr<PerfectNSet<clingo_literal_t>> targetsDerivedFromAssumptions;
    Vector<clingo_literal_t> assumptions;
    // unordered_set<clingo_literal_t> assumptionsSlitIds;
    std::unique_ptr<PerfectNSet<clingo_literal_t>> assumptionsSlitIds;
    std::unique_ptr<PerfectNHash<clingo_literal_t, std::vector<clingo_literal_t>>> mapTargetsDerivedFromAssumptionsSlits;
};

} // namespace fiore
