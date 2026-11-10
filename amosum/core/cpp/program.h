#pragma once
#include <iostream>
#include <clingo.h>
#include <vector>
#include <memory>
#include <unordered_set>
#include "amosum/core/cpp/utility.h"
#include "fiore/utils/logger.h"
#include "fiore/model/program.h"
#include "fiore/model/assigment.h"      // fiore::ClingoAssignment, fiore::AssignmentType

enum class AmosumPrintMode{ EachModel, LastModel, CostOnly, ProgressionLastModel };
AmosumPrintMode amosumPrintMode() noexcept;         

struct ProgramGroup{
    std::string id; // id of the group
    std::vector<const fiore::Literal*> members; // members of the group
    bool aux = false; // if it is the aux atom of the amosum construct
};

struct AmosumConstraint: public fiore::PBConstraint{
    std::string id;                                     // aggregate id: last argument of __group__
    std::string type;                                   // ge_amo, ge_eo, le_amo, le_eo, amomaximize (from __aux__)
    bool ge;                                            // __lb__ (>=) or __ub__ (<=)
    std::vector<ProgramGroup> groups;                   // after the program level normalization (AmosumGrounder)
    std::vector<clingo_literal_t> groupsAtoms;          // the __group__ atoms themselves

    AmosumConstraint(const fiore::Program* program, int64_t bound, fiore::Vector<const fiore::Literal*>* literals, fiore::UMNWeightFunction* weightFunction) noexcept
        : fiore::PBConstraint(program, bound, literals, weightFunction), ge(true){}

    inline int64_t weight(const fiore::Literal* l) const noexcept { return weightFunction->at(l->id); }
    inline std::string toString() const noexcept override{ std::ostringstream out; out<<"AmosumConstraint("<<id<<", "<<type<<", "<<groups.size()<<" groups, "<<(ge ? ">= " : "<= ")<<bound<< ")"; return out.str();};
};



class AmosumProgram: public fiore::Program{
public:
    std::vector<AmosumConstraint*> amosumConstraints;   // non-owning: the constraints are owned by pBConstraints
    AmosumConstraint* amomaximizeConstraint;
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
    // the #amomaximize value only when there is one; cost-only: no atoms; the caller's prefix is kept
    void printAssignment(std::string prefix = "") noexcept override;
    // the last line of cost-only: the value and whether it is proven optimal
    std::string costLine() const noexcept;
};
