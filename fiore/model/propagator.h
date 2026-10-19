#pragma once
#include "../utils/utility.h"
#include "../clingo/utilityClingo.h"
#include "../utils/logger.h"



namespace fiore {

struct ClingoPropagator;
struct ClingoSolver;

struct ClingoPropagatorInitializer{

    // inline static ClingoPropagatorInitializer* getInstance() noexcept{ if(instance == nullptr) instance = new ClingoPropagatorInitializer(); return instance;}
    // inline static void cleanup(){if(instance == nullptr) delete instance;}

    void init(clingo_propagate_init* _init, ClingoSolver* propagator) noexcept;
    ClingoPropagatorInitializer(): nt(0), first(true){}
    
    // std::vector<clingo_literal_t> solvingLiteralFactIds;
    std::vector<const SolvingLiteral*> globalSolvingLiterals;
    std::unique_ptr<MapLiteralId2Literal> mapSlitId2Slit;
    size_t nt;
    size_t maxSolverLiteralId;

    inline void initiliazeAgain(){ first = true; }
    Vector<const Literal*> programAtomRestNoFact;
    Vector<const Literal*> factsVectorProgramLiterals;
    std::unordered_map<clingo_literal_t, clingo_literal_t> mapPlit2SlitAssumptions;
    inline bool isFact(clingo_literal_t plit) noexcept { return factsSetProgramLiterals.find(plit) != factsSetProgramLiterals.end(); }
private:
    std::unordered_set<clingo_literal_t> factsSetProgramLiterals;
    bool first;
    // inline void ClingoPropagatorInitializer::assignPropagatorFields(ClingoPropagator* propagator) noexcept{ propagator->global = this;}
    // static ClingoPropagatorInitializer* instance;
};

bool initImpDecisionWeakPropagatorAbstraction(clingo_propagate_init_t *init, ClingoPropagator* propagator);

struct ClingoPropagator{

    ClingoPropagator(std::string name, ClingoSolver* const solver):solver(solver), firstTime(true), name(name), enabled(true){}

    //! This function is called once before each solving step.
    //! It is used to map relevant program literals to solver literals, add watches for solver literals, and initialize
    //! the data structures used during propagation.
    //!
    //! @note This is the last point to access symbolic and theory atoms.
    //! Once the search has started, they are no longer accessible.
    //!
    //! @param[in] init initialization object
    //! @param[in] data user data for the callback
    //! @return whether the call was successful
    //! @see ::clingo_propagator_init_callback_t
    bool (*init)(clingo_propagate_init_t *init, ClingoPropagator* propagator) = nullptr;

    //! Can be used to propagate solver literals given a @link clingo_assignment_t partial assignment@endlink.
    //!
    //! Called during propagation with a non-empty array of @link clingo_propagate_init_add_watch() watched solver
    //! literals@endlink that have been assigned to true since the last call to either propagate, undo, (or the start of
    //! the search) - the change set. Only watched solver literals are contained in the change set. Each literal in the
    //! change set is true w.r.t. the current @link clingo_assignment_t assignment@endlink.
    //! @ref clingo_propagate_control_add_clause() can be used to add clauses.
    //! If a clause is unit resulting, it can be propagated using @ref clingo_propagate_control_propagate().
    //! If the result of either of the two methods is false, the propagate function must return immediately.
    //!
    //! The following snippet shows how to use the methods to add clauses and propagate consequences within the
    //! callback. The important point is to return true (true to indicate there was no error) if the result of either of
    //! the methods is false.
    //! ~~~~~~~~~~~~~~~{.c}
    //! bool result;
    //! clingo_literal_t clause[] = { ... };
    //!
    //! // add a clause
    //! if (!clingo_propagate_control_add_clause(control, clause, clingo_clause_type_learnt, &result) { return false; }
    //! if (!result) { return true; }
    //! // propagate its consequences
    //! if (!clingo_propagate_control_propagate(control, &result) { return false; }
    //! if (!result) { return true; }
    //!
    //! // add further clauses and propagate them
    //! ...
    //!
    //! return true;
    //! ~~~~~~~~~~~~~~~
    //!
    //! @note
    //! This function can be called from different solving threads.
    //! Each thread has its own assignment and id, which can be obtained using @ref
    //! clingo_propagate_control_thread_id().
    //!
    //! @param[in] control control object for the target solver
    //! @param[in] changes the change set
    //! @param[in] size the size of the change set
    //! @param[in] data user data for the callback
    //! @return whether the call was successful
    //! @see ::clingo_propagator_propagate_callback_t
    bool (*propagate)(clingo_propagate_control_t *control, clingo_literal_t const *changes, size_t size, ClingoPropagator* propagator) = nullptr;

    //! Called whenever a solver undoes assignments to watched solver literals.
    //!
    //! This callback is meant to update assignment dependent state in the propagator.
    //!
    //! @note No clauses must be propagated in this callback and no errors should be set.
    //!
    //! @param[in] control control object for the target solver
    //! @param[in] changes the change set
    //! @param[in] size the size of the change set
    //! @param[in] data user data for the callback
    //! @return whether the call was successful
    //! @see ::clingo_propagator_undo_callback_t
    void (*undo)(clingo_propagate_control_t const *control, clingo_literal_t const *changes, size_t size, ClingoPropagator* propagator) = nullptr;

    //! This function is similar to @ref clingo_propagate_control_propagate() but is called without a change set on
    //! propagation fixpoints.
    //!
    //! When exactly this function is called, can be configured using the @ref clingo_propagate_init_set_check_mode()
    //! function.
    //!
    //! @note This function is called even if no watches have been added.
    //!
    //! @param[in] control control object for the target solver
    //! @param[in] data user data for the callback
    //! @return whether the call was successful
    //! @see ::clingo_propagator_check_callback_t
    bool (*check)(clingo_propagate_control_t *control, ClingoPropagator* propagator) = nullptr;

    //! This function allows a propagator to implement domain-specific heuristics.
    //!
    //! It is called whenever propagation reaches a fixed point and
    //! should return a free solver literal that is to be assigned true.
    //! In case multiple propagators are registered,
    //! this function can return 0 to let a propagator registered later make a decision.
    //! If all propagators return 0, then the fallback literal is
    //!
    //! @param[in] thread_id the solver's thread id
    //! @param[in] assignment the assignment of the solver
    //! @param[in] fallback the literal chosen by the solver's heuristic
    //! @param[out] decision the literal to make true
    //! @return whether the call was successful
    bool (*decide)(clingo_id_t thread_id, clingo_assignment_t const *assignment, clingo_literal_t fallback, ClingoPropagator* propagator, clingo_literal_t *decision) = nullptr;

    // virtual const std::vector<const Literal*>& softLiterals() = 0;
    virtual const Vector<const Literal*>& programLiterals() = 0;
    // virtual UMNWeightFunction& weightFunctionProgramLiterals() = 0;


    ClingoSolver* const  solver;
    std::vector<const SolvingLiteral*> localSolvingLiterals;


    friend bool initImpDecisionWeakPropagatorAbstraction(clingo_propagate_init_t *init, ClingoPropagator* propagator);

    #ifdef __debug__
        inline std::unordered_map<std::string, double> getStatistics() const { return stats; }
        inline void resetTimes(){ for(auto&[key, value]: this->stats) value = 0; }
        inline void increaseStat(std::string key, double increase){ 
            auto it = this->stats.find(key); 
            it == this->stats.end() ? this->stats[key] = increase : it->second += increase;
        }
    #endif
    inline void setName(std::string name){ this->name = name; };
    inline std::string getName() const { return this->name; };
    virtual ~ClingoPropagator() = default;
    inline void enable() noexcept{ enabled = true;}
    inline void disable() noexcept{ enabled = false;}
    virtual inline bool isEnabled() const noexcept{ return enabled;}
    
    bool clearIfNotEnabledInInit(clingo_propagate_init* init);
    std::vector<const Literal*> solvingLiteralIdsToSolvingLiterals(clingo_literal_t const *changes, size_t size)noexcept;
    inline std::vector<const Literal*> solvingLiteralIdsToSolvingLiterals(const std::vector<clingo_literal_t>& changes)noexcept{ return solvingLiteralIdsToSolvingLiterals(changes.data(), changes.size()); }
    std::vector<const Literal*> solvingLiteralIdsToSolvingLiterals(const std::unordered_set<clingo_literal_t>& changes)noexcept;
    inline const Literal* solvingLiteralIdToSolvingLiteral(const clingo_literal_t& lit)noexcept{ return solvingLiteralIdsToSolvingLiterals(&lit, 1)[0]; }
    void removeWatchAll(clingo_propagate_init* init);

protected: 
    virtual void initFirstTime(clingo_propagate_init* _init);
    #ifdef __debug__
        std::unordered_map<std::string, double> stats;
    #endif
    bool firstTime;
    std::string name;
    bool enabled;
    std::vector<clingo_literal_t> watchedLiterals;
};
        
std::ostream& operator<<(std::ostream& out, const ClingoPropagator& p);

} // namespace fiore
