#pragma once
#include "../utils/utility.h"
#include "../clingo/utilityClingo.h"
#include "../utils/logger.h"



namespace fiore {

enum AssignmentType{BraveConsequences=0, CautiousConsequences, StableModel, RandomAssignment, NoSolution};

struct Program;

struct Assignment{

    const Program* program;
    size_t number ;
    size_t id ;
    bool optimalityProven;
    const AssignmentType type;
    std::vector<const Literal*> trueSoftLiterals;
    clingo::ClingoResult result;
    double time;
    int64_t costProgram;
    std::string outputString;
    const size_t seed;
    

    std::string toString(bool printDefault = false)const noexcept;
    std::string toAnswerSetString(const Vector<const Literal*> *toShow = nullptr)const noexcept;
    std::string toInterpretationString(bool printUndefined = false,  bool onlyTrueAtoms = true, Vector<const Literal*> *toShow = nullptr);
    std::string toConstraints()const noexcept;
    std::string toOutputString(Vector<const Literal*> *toShow = nullptr, const fiore::UMNWeightFunction* w = nullptr) noexcept;
    std::string toCostOutputString(Vector<const Literal*> *toShow = nullptr) noexcept;

    void foundNewAnswerset(std::string prefix = "", const std::string& verbose = CONSTANTS::verboseLevel1) noexcept;
    virtual void printAssignment(std::string prefix = "") noexcept;
    void printCostAssignment(std::string prefix = "") noexcept;
    void setAsCurrentOptimum() noexcept;

    inline bool isOptimum()const noexcept{ return this->result.isOptimum();}
    inline bool isSolutionUnkown()const noexcept{ return this->type == AssignmentType::NoSolution;}

    inline bool isTrue(const clingo_literal_t& literalId)const { return this->assignment->isTrue(literalId);}
    inline bool isTrue(const Literal* literal)const { return this->isTrue(literal->id);}
    
    inline bool isFalse(const clingo_literal_t& literalId)const { return this->assignment->isFalse(literalId);}
    inline bool isFalse(const Literal* literal)const { return this->isFalse(literal->id);}

    inline bool isUndefined(const clingo_literal_t& literalId)const { return this->assignment->isUndef(literalId);}
    inline bool isUndefined(const Literal* literal)const { return this->isUndefined(literal->id);}

    inline void setTime(double time) noexcept { this->time = time; }
    int64_t cost(UMNWeightFunction* weight = nullptr) noexcept;
    float asymmetricInclusionDistance(const Assignment& other) noexcept;
    // Overlap Distance (Szymkiewicz–Simpson Distance)
    float overlapDistance(const Assignment& other) noexcept;

    std::vector<int32_t> convertIntoAssumptions(const Vector<const Literal*>* literals = nullptr) noexcept;

    // std::vector<const Literal *> clingo::trueLiterals(const Vector<const Literal*>* literals = nullptr) noexcept;
    size_t numberOfTrueLiterals(const Vector<const Literal*>* literals = nullptr) const noexcept;
    inline bool isCostInfinite()const noexcept{ return costProgram == CONSTANTS::INFPlus64;} 

    virtual ~Assignment();
    
    inline void set(const int32_t& literalId,  const char value) noexcept { this->assignment->set(literalId, value); }

    static const Assignment* randomAssignmentForSoft(const Program* program, double p);
    inline static Assignment* getNoSolutionAssignment() noexcept { return new Assignment(); } 
protected:
    Assignment(const Program* program, const size_t number, bool optimalityProven = false, const AssignmentType type = AssignmentType::StableModel, double time = 0) noexcept;
private:
    std::unique_ptr<UMInterpretation> assignment;
    Assignment(const Program* program) noexcept;
    Assignment() noexcept;
};

struct ClingoAssignment: public Assignment{
    int64_t* clingoCost;
    // avoidToShow: atoms not printed (nullptr: all shown); not owned, it must outlive the assignment
    ClingoAssignment(Program* program, const size_t number, const clingo_model_t* model, const AssignmentType type = AssignmentType::StableModel, const std::regex* avoidToShow = nullptr);
    ClingoAssignment(Program* program, const clingo_model_t* model, const AssignmentType type = AssignmentType::StableModel, const std::regex* avoidToShow = nullptr);
    void init(const clingo_model_t* model);
    const std::regex* const avoidToShowRegex;
};

std::ostream& operator<<(std::ostream& out, const Assignment& value);

struct AnswersetsPool{

    int numberOfNotDeletedAnswerset = 0;

    inline size_t addAnswerset(Assignment* assignment){ if(assignment != nullptr) assignment->id = autoIncId++; this->answersets[assignment->id] = assignment; return assignment->id;}
    inline void deleteAnswerset(Assignment* assignment){ if(assignment != nullptr) deleteAnswerset(assignment->id);}
    inline void setOptimalAnswerSet(Assignment* assignment){ 
        if(assignment != nullptr) {
            setOptimalAnswerSet(assignment->id);
            #ifdef __debug__
                LOG(LogLevel::INFO_1, "Setting as optimum answer set: ", assignment->id);
            #endif
        } 
    }
    
    inline void deleteAnswerset(size_t id){ 
        auto it = this->answersets.find(id);
        if(it == this->answersets.end()){
            #ifdef __debug__
                LOG(LogLevel::ERROR, id, " not present in answersets id");
            #endif
            return;
        }
        Assignment* ass = it->second; 
        if(ass != nullptr){
            delete this->answersets[id];
            #ifdef __debug__
                LOG(LogLevel::INFO, "Deliting answer set: ", id);
            #endif
        }
        this->answersets[id] = nullptr; 
    }
    inline void setOptimalAnswerSet(size_t id){ 
        optimalAnswerSet = id; 
        #ifdef __debug__
            LOG(LogLevel::INFO, "Setting as optimum answer set: ", id);
        #endif
    }

    inline Assignment* getAnswerset(size_t id){ return this->answersets[id]; }
    inline Assignment* getOptimalAnswerSet(){ return optimalAnswerSet != CONSTANTS::NONE32 ? this->answersets[optimalAnswerSet]: Assignment::getNoSolutionAssignment();}
    static AnswersetsPool* getInstance() noexcept;
    inline static void cleanup(){ delete instance; }
    inline size_t getNextIdAnswersetPrinted() noexcept{ return autoIncIdForPrinting++; }
    inline size_t getCurrentIdAnswersetPrinted() noexcept{ return autoIncIdForPrinting; }
    inline ~AnswersetsPool(){
        for(auto&[key, value]: this->answersets){
            if(value != nullptr) delete value;
        }
    };

    int64_t upperBound;

private:
    std::unordered_map<size_t, Assignment*> answersets;
    size_t autoIncId;
    size_t autoIncIdForPrinting;
    size_t optimalAnswerSet;
    static AnswersetsPool* instance;
    AnswersetsPool() noexcept: upperBound(CONSTANTS::INFPlus64), autoIncId(0), autoIncIdForPrinting(1), optimalAnswerSet(CONSTANTS::NONE32){}
};

} // namespace fiore
