#pragma once
#include <clingo.h>
#include "../utils/utilityCommon.h"
#include "../utils/logger.h"




// clingo errors: with __debug__ the error is logged with its location (see handle_error)
#ifdef __debug__
    #define handleError(success)\
    ::fiore::clingo::handle_error(success, __FILE__, __LINE__)
#else
    #define handleError(success)\
    success
#endif

namespace fiore::clingo {

extern const std::vector<clingo_literal_t> EMPTY_LITERALS;

struct ClingoResult{

    enum ClingoSolveValue {
        clingo_solve_value_unkown = 0,
        clingo_solve_value_satisfiable = 1,
        clingo_solve_value_unsatisfiable = 2,
        clingo_solve_value_exhausted = 4,
        clingo_solve_value_interrupted = 8,
        clingo_solve_value_optimum = 16,
        clingo_solve_value_unkown_optimum = 32,
        clingo_solve_value_error = 64,
        clingo_solve_no_solution_found = 128
    };

    size_t value;
    bool solvedAtLevel0;
    inline short exitCode() const noexcept {
        if (this->isSolutionUnkown()) return CONSTANTS::UNKOWN_CODE;      // "No solution found": no answer set known
        if (this->isUnsat()) return CONSTANTS::UNSAT_CODE;
        if (this->isOptimum()) return CONSTANTS::OPTIMUM_CODE;
        if (this->isOptimumUnkownSat()) return CONSTANTS::OPTIMUM_UNKOWN_CODE;
        if (this->isOptimumUnkown()) return CONSTANTS::UNKOWN_CODE;
        if (this->isSat()) return CONSTANTS::SAT_CODE;
        if (this->isUnkown()) return CONSTANTS::UNKOWN_CODE;
        return CONSTANTS::ERROR_CODE;
    }

    ClingoResult(size_t result = clingo_solve_value_error, bool solvedAtLevel0 = false)noexcept: value(result), solvedAtLevel0(solvedAtLevel0){}

    ClingoResult(ClingoSolveValue result, bool solvedAtLevel0 = false)noexcept: value(result){}

    std::string toString() const noexcept{
        std::stringstream out;
        if(this->isSat()) out<<std::string("SAT");
        else if (this->isUnsat()) out<<std::string("UNSAT");
        else if (this->isUnkown()) out<<std::string("UNKOWN");
        else out<<std::string("ERROR");
    
        return out.str();
    }

    inline void optimum(){ if(value == clingo_solve_no_solution_found) return; value |= clingo_solve_value_optimum;        value &= ~(size_t)clingo_solve_value_unkown_optimum; sat(); }
    inline void unkown(){  if(value == clingo_solve_no_solution_found) return; value |= clingo_solve_value_unkown_optimum; value &= ~(size_t)clingo_solve_value_optimum; }
    inline void unsat(){   if(value == clingo_solve_no_solution_found) return; value |= clingo_solve_value_unsatisfiable;  value &= ~(size_t)clingo_solve_value_satisfiable; }
    inline void sat(){     if(value == clingo_solve_no_solution_found) return; value |= clingo_solve_value_satisfiable;    value &= ~(size_t)clingo_solve_value_unsatisfiable; }
    
    inline bool isSat() const noexcept{ return this->value & clingo_solve_value_satisfiable; }
    inline bool isOptimum() const noexcept{ return this->value & clingo_solve_value_optimum; }
    inline bool isOptimumUnkown() const noexcept{ return this->value & clingo_solve_value_unkown_optimum; }
    inline bool isOptimumUnkownSat() const noexcept{ return this->isOptimumUnkown() && this->isSat(); }
    inline bool isUnkown() const noexcept{ return this->value == clingo_solve_value_unkown; }
    inline bool isUnsat() const noexcept{ return this->value & clingo_solve_value_unsatisfiable; }
    inline bool isSolutionUnkown() const noexcept{ return this->value & clingo_solve_no_solution_found; }

};

std::ostream& operator<<(std::ostream& out, const ClingoResult& clingoResult);
void clingoLogger(clingo_warning_t code, char const *message, void *data);

std::string from_symbol_to_string(clingo_symbol_t symbol) noexcept;

void handle_error(bool success, const char* file, size_t line) ;

inline bool isTrue(const clingo_assignment_t *assignment, clingo_literal_t slit)noexcept{
    bool res ;
    clingo_assignment_is_true(assignment, slit, &res);
    return res ;
}

inline bool isFalse(const clingo_assignment_t *assignment, clingo_literal_t slit)noexcept{
    bool res ;
    clingo_assignment_is_false(assignment, slit, &res);
    return res ;
}

inline bool isUndef(const clingo_assignment_t *assignment, clingo_literal_t slit) noexcept{ return !isFalse(assignment, slit) && !isTrue(assignment, slit) ; }


void setVSIDSHeuristic(clingo_control_t* control);

inline std::vector<const SolvingLiteral*> undefinedLiterals(const clingo_assignment_t *assignment, const std::vector<const SolvingLiteral*>& literals) noexcept{
    std::vector<const SolvingLiteral*> undefinedLits;
    for(const SolvingLiteral* l: literals) if(isUndef(assignment, l->id)) undefinedLits.push_back(l);
    return undefinedLits;
}

inline std::vector<const SolvingLiteral*> trueLiterals(const clingo_assignment_t *assignment, const std::vector<const SolvingLiteral*>& literals) noexcept{
    std::vector<const SolvingLiteral*> trueLits;
    for(const SolvingLiteral* l: literals) if(isTrue(assignment, l->id)) trueLits.push_back(l);
    return trueLits;
}

inline std::vector<const SolvingLiteral*> falseLiterals(const clingo_assignment_t *assignment, const std::vector<const SolvingLiteral*>& literals) noexcept{
    std::vector<const SolvingLiteral*> falseLits;
    for(const SolvingLiteral* l: literals) if(isFalse(assignment, l->id)) falseLits.push_back(l);
    return falseLits;
}

inline std::unordered_map<const SolvingLiteral, TruthValue, Hasher>truthValuesOfLits(const clingo_assignment_t *assignment, const std::vector<const SolvingLiteral*>& literals) noexcept{
    std::unordered_map<const SolvingLiteral, TruthValue, Hasher> truthValues;
    for(const SolvingLiteral* l: literals) {
        if(isFalse(assignment, l->id)) truthValues[*l] = TruthValue(TruthValue::F);
        else if(isTrue(assignment, l->id)) truthValues[*l] = TruthValue(TruthValue::T);
        else truthValues[*l] = TruthValue(TruthValue::U);
    }
    return truthValues;
}

bool getMapKeyStatisticClingo(const clingo_statistics_t *stats, uint64_t parent_key, std::string name, uint64_t *out_key);
double getValueStatisticClingo(const clingo_statistics_t *stats, std::string path);
void setValueConfigurationClingo(clingo_configuration_t *conf, std::string path, std::string value);
bool printClingoStatisticClingo(const clingo_statistics_t *stats, uint64_t key, int depth=0);
bool printConfigurationClingo(const clingo_configuration_t *conf, clingo_id_t key, int depth=0);

} // namespace fiore::clingo
