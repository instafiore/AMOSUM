#pragma once
#include "../utils/utility.h"
#include "propagator.h"
#include "program.h"


namespace fiore {

struct PropagatorPBConstraintImplementation;
struct ClingoSolver;

bool initImplementation(clingo_propagate_init_t *init, ClingoPropagator* prop);
bool propagateImplementation(clingo_propagate_control_t *control, clingo_literal_t const *changes, size_t size, ClingoPropagator* propagator);
void undoImplementation(clingo_propagate_control_t const *control, clingo_literal_t const *changes, size_t size, ClingoPropagator* propagator);


struct PropagatorPBConstraint: public ClingoPropagator{

    PropagatorPBConstraint(std::string name, ClingoSolver* solver, PBConstraint* pbConstraint, const bool& watchOppositePolarityLiterals = false): ClingoPropagator(name, solver, watchOppositePolarityLiterals),
    pbConstraint(pbConstraint) {
        this->init = &initImplementation;
        this->propagate = &propagateImplementation;
        this->undo = &undoImplementation;
    }
    
    // inline const UMNVector<const Literal*>& programLiterals() override { return plits.get() != nullptr ? *plits.get() : *solver->program->literals.get(); }
    
    friend bool initImplementation(clingo_propagate_init_t *init, ClingoPropagator* prop);
    friend bool propagateImplementation(clingo_propagate_control_t *control, clingo_literal_t const *changes, size_t size, ClingoPropagator* propagator);
    friend void undoImplementation(clingo_propagate_control_t const *control, clingo_literal_t const *changes, size_t size, ClingoPropagator* propagator);

    inline std::string toString() const { std::ostringstream o; o<<"PropagatorPBConstraintAbstract("<<name<<")"; return o.str();}
    bool addClausesPropagatedLitIds(const clingo_propagate_control_t *control, const std::vector<clingo_literal_t>& propagatedLiteralIds) noexcept;
    bool addClause(const clingo_propagate_control_t *control, const std::vector<clingo_literal_t>& clause) noexcept;
     
    inline bool isEnabled() const noexcept override{ return ClingoPropagator::isEnabled() && pbConstraint->bound != CONSTANTS::INFPlus64; }

    inline const Vector<const Literal*>& programLiterals() override {return *pbConstraint->ownedLiterals;}
    virtual ~PropagatorPBConstraint();
    PBConstraint* pbConstraint;
    std::unique_ptr<PerfectWeightFunction> weightSolvingLiterals;

    
    std::vector<PropagatorPBConstraintImplementation*> implementations;
    // friend ClingoSolver;
protected:
    virtual PropagatorPBConstraintImplementation* createImplementation() noexcept;
    virtual void initFirstTime(clingo_propagate_init* _init) noexcept override;
    size_t maxSolvingLiteralId;

};


inline std::ostream& operator<<(std::ostream& out, const PropagatorPBConstraint& pb){ return out<<pb.toString();}

} // namespace fiore
