#pragma once
#include <iostream>
#include <clingo.h>
#include <vector>
#include <memory>
#include <unordered_set>
#include "utility.h"
#include "fiore/utils/logger.h"
#include "fiore/model/program.h"
#include "fiore/model/assigment.h"      // fiore::ClingoAssignment, fiore::AssignmentType


// a group of an AmosumConstraint over program literals (from the grounding): at most one member true (exactly one for EO)
struct ProgramGroup{
    std::string id;
    std::vector<std::pair<const fiore::Literal*, int64_t>> members;   // program literal, weight (0: counted in another group)
    // the group of the "-" element (¬__aux__, weight = bound): it changes only when the aggregate's body does, so it is left
    // out of the lazy threshold (otherwise the threshold is always met)
    bool aux = false;
};
struct AmosumConstraint: public fiore::PBConstraint{
    // the aggregate is described by `groups` only: PBConstraint's literals and weight function are empty (owned here,
    // as PBConstraint keeps raw pointers) and must not be used for amosum
    std::string id;                                     // aggregate id: last argument of __group__
    std::string type;                                   // ge_amo, ge_eo, le_amo, le_eo, amomaximize (from __aux__)
    bool ge;                                            // __lb__ (>=) or __ub__ (<=)
    std::vector<ProgramGroup> groups;                   // after the program level normalization (AmosumGrounder)
    std::vector<clingo_literal_t> groupsAtoms;          // the __group__ atoms themselves

    AmosumConstraint(const fiore::Program* program, int64_t bound, fiore::Vector<const fiore::Literal*>* literals, fiore::UMNWeightFunction* weightFunction) noexcept 
        : fiore::PBConstraint(program, bound, literals, weightFunction), ge(true){}

    inline std::string toString() const noexcept override{ std::ostringstream out; out<<"AmosumConstraint("<<id<<", "<<type<<", "<<groups.size()<<" groups, "<<(ge ? ">= " : "<= ")<<bound<< ")"; return out.str();};
};



class AmosumProgram: public fiore::Program{
public:
    std::vector<AmosumConstraint*> amosumConstraints;   // non-owning: the constraints are owned by pBConstraints
    AmosumConstraint* amomaximizeConstraint;
    // owners of the (empty) literals / weight function of each AmosumConstraint: PBConstraint keeps raw pointers
    std::vector<std::unique_ptr<fiore::Vector<const fiore::Literal*>>> pbLiterals;
    std::vector<std::unique_ptr<fiore::UMNWeightFunction>> pbWeightFunctions;
    struct AmosumGrounder: fiore::Program::ProgramGrounder{
        fiore::Program* ground(const std::vector<std::string>& ASPInputs) override;
    protected:
        fiore::Program* createProgram() const override;
    };

    AmosumProgram() = default;
};

struct AmosumClingoAssignment: public fiore::ClingoAssignment{
    int64_t amosumCost = 0;         // value of the #amomaximize in this model (0 without it)
    AmosumClingoAssignment(AmosumProgram* program, const size_t number, const clingo_model_t* model, const fiore::AssignmentType type = fiore::AssignmentType::StableModel):
        fiore::ClingoAssignment(program, number, model, type, &SETTINGS::auxRegex){ initAmosum(model); }
    AmosumClingoAssignment(AmosumProgram* program, const clingo_model_t* model, const fiore::AssignmentType type = fiore::AssignmentType::StableModel):
        fiore::ClingoAssignment(program, model, type, &SETTINGS::auxRegex){ initAmosum(model); }
    void initAmosum(const clingo_model_t* model);
    // the #amomaximize value only when there is one; the caller's prefix is kept
    void printAssignment(std::string prefix = "") noexcept override {
        const AmosumProgram* p = static_cast<const AmosumProgram*>(this->program);
        if(p->amomaximizeConstraint != nullptr) prefix += "Amomaximize Cost: " + std::to_string(this->amosumCost) + " ";
        fiore::ClingoAssignment::printAssignment(prefix);
    }
};