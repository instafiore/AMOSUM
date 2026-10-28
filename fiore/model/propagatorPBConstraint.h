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
bool checkImplementation(clingo_propagate_control_t *control, ClingoPropagator* propagator);


struct PropagatorPBConstraint: public ClingoPropagator{

    PropagatorPBConstraint(std::string name, ClingoSolver* solver, PBConstraint* pbConstraint, const bool& watchOppositePolarityLiterals = false, const clingo_clause_type_t clauseType = clingo_clause_type_learnt): ClingoPropagator(name, solver, watchOppositePolarityLiterals),
    pbConstraint(pbConstraint), clauseTypeGlobal(clauseType){
        this->init = &initImplementation;
        this->propagate = &propagateImplementation;
        this->undo = &undoImplementation;
        // only for the init conflicts of a volatile clause type (see initPropagation): the check mode is none otherwise
        this->check = &checkImplementation;
    }
    
    // inline const UMNVector<const Literal*>& programLiterals() override { return plits.get() != nullptr ? *plits.get() : *solver->program->literals.get(); }
    
    friend bool initImplementation(clingo_propagate_init_t *init, ClingoPropagator* prop);
    friend bool propagateImplementation(clingo_propagate_control_t *control, clingo_literal_t const *changes, size_t size, ClingoPropagator* propagator);
    friend void undoImplementation(clingo_propagate_control_t const *control, clingo_literal_t const *changes, size_t size, ClingoPropagator* propagator);
    friend bool checkImplementation(clingo_propagate_control_t *control, ClingoPropagator* propagator);

    inline std::string toString() const { std::ostringstream o; o<<"PropagatorPBConstraintAbstract("<<name<<")"; return o.str();}
    void initPropagation(clingo_propagate_init_t *init) noexcept;
    bool addClausesPropagatedLitIds(const clingo_propagate_init *control, const std::vector<clingo_literal_t>& propagatedLiteralIds, const size_t td) noexcept { return _addClausesPropagatedLitIds(control, propagatedLiteralIds, true, td); }
    bool addClausesPropagatedLitIds(const clingo_propagate_control *control, const std::vector<clingo_literal_t>& propagatedLiteralIds, const clingo_clause_type_t clauseType = CONSTANTS::NONE32) noexcept;
    inline bool addClause(const clingo_propagate_init* control, const std::vector<clingo_literal_t>& clause) noexcept{ return _addClause(control, clause, true);}
    inline bool addClause(const clingo_propagate_control* control, const std::vector<clingo_literal_t>& clause, const clingo_clause_type_t clauseType = CONSTANTS::NONE32) noexcept{ return _addClause(control, clause, false, clauseType);}
  
    inline bool isEnabled() const noexcept override{ return ClingoPropagator::isEnabled() && pbConstraint->bound != CONSTANTS::INFPlus64; }

    inline const Vector<const Literal*>& programLiterals() override {return *pbConstraint->ownedLiterals;}
    virtual ~PropagatorPBConstraint();
    PBConstraint* pbConstraint;
    std::unique_ptr<PerfectWeightFunction> weightSolvingLiterals;

    inline void updateClauseTypeGlobal(clingo_clause_type_t clauseTypeGlobal) noexcept{ this->clauseTypeGlobal = clauseTypeGlobal; }
    // volatile clauses are deleted after the current solve call: for a bound that can also decrease between calls
    inline bool isVolatileClauseType() const noexcept{ return clauseTypeGlobal == clingo_clause_type_volatile || clauseTypeGlobal == clingo_clause_type_volatile_static; }
    std::vector<PropagatorPBConstraintImplementation*> implementations;
    // friend ClingoSolver;
protected:
    virtual PropagatorPBConstraintImplementation* createImplementation() noexcept;
    virtual void initFirstTime(clingo_propagate_init* _init) noexcept override;
    size_t maxSolvingLiteralId;

    bool _addClause(const void *control, const std::vector<clingo_literal_t>& clause, const bool isInitControl, const clingo_clause_type_t clauseType = CONSTANTS::NONE32) noexcept;
    bool _addClausesPropagatedLitIds(const void *control, const std::vector<clingo_literal_t>& propagatedLiteralIds, const bool isInitControl, const size_t td, const clingo_clause_type_t clauseType = CONSTANTS::NONE32) noexcept;
    clingo_clause_type_t clauseTypeGlobal;
    // volatile clause type: the clauses of initPropagation, one list per thread, added by checkImplementation at the first
    // fixpoint of the solve call (clauses added at init are permanent, while these hold only for the current bound)
    std::vector<std::vector<std::vector<clingo_literal_t>>> pendingInitClauses;


};


inline std::ostream& operator<<(std::ostream& out, const PropagatorPBConstraint& pb){ return out<<pb.toString();}

} // namespace fiore
