#include "assigment.h"
#include "program.h"
#include "../utils/stats.h"
#include "../utils/argparse.h"


namespace fiore {

std::ostream& operator<<(std::ostream& out, const Assignment& ass){ return out<<ass.toString();}


Assignment::Assignment(const Program* program, const size_t number, bool optimalityProven, const AssignmentType type, double time)noexcept:
        program(program),number(number), optimalityProven(optimalityProven), type(type), 
        result(clingo::ClingoResult::ClingoSolveValue::clingo_solve_value_satisfiable), time(time), outputString(""), seed(std::stoi(ArgParse::getInstance()->get(CONSTANTS::seedKey))), assignment(new UMInterpretation(program->maxOriginLiteralId + 1, false)){
    #ifdef __debug__
        int n = ++AnswersetsPool::getInstance()->numberOfNotDeletedAnswerset;
        LOG(LogLevel::DEBUG, "Creating answer set, free answer sets: ",n);
    #endif

}

Assignment::Assignment()noexcept:
        program(nullptr),number(CONSTANTS::NONE32), optimalityProven(false), type(AssignmentType::NoSolution), 
        result(clingo::ClingoResult::ClingoSolveValue::clingo_solve_no_solution_found), time(CONSTANTS::NONE32), costProgram(CONSTANTS::INFPlus64), outputString("No solution found"), seed(std::stoi(ArgParse::getInstance()->get(CONSTANTS::seedKey))), assignment(nullptr){
    #ifdef __debug__
        int n = ++AnswersetsPool::getInstance()->numberOfNotDeletedAnswerset;
        LOG(LogLevel::DEBUG, "Creating answer set, free answer sets: ",n);
    #endif

}


ClingoAssignment::ClingoAssignment(Program* program, const size_t number, const clingo_model_t* model, const AssignmentType type, const std::regex* avoidToShow):
    Assignment(program, number, type), avoidToShowRegex(avoidToShow){
    this->init(model);
}

ClingoAssignment::ClingoAssignment(Program* program, const clingo_model_t* model, const AssignmentType type, const std::regex* avoidToShow):
    Assignment(program, CONSTANTS::NONE32, type), avoidToShowRegex(avoidToShow){
    this->init(model);
}


Assignment::Assignment(const Program* program)noexcept: program(program), number(-1), optimalityProven(false), type(AssignmentType::RandomAssignment),
result(clingo::ClingoResult::ClingoSolveValue::clingo_solve_value_unkown), time(-1), outputString(""), seed(std::stoi(ArgParse::getInstance()->get(CONSTANTS::seedKey))), assignment(new UMInterpretation(program->maxOriginLiteralId + 1, false)){

}

const Assignment* Assignment::randomAssignmentForSoft(const Program* program, double p){
    Assignment* assignment = new Assignment(program);

    for(const Literal* l: *program->softLiterals.get()){
        const bool isTrue = random01(assignment->seed) > p;
        assignment->set(l->id, isTrue);
    }

    return assignment;
}

bool isSoftLiteralOf(const Program* program, clingo_literal_t literalId) noexcept{ return program->isSoft(literalId); }

void ClingoAssignment::init(const clingo_model_t* model){
    // the shown atoms, as symbols: no text here
    ensuredRestLiterals = false;  
    size_t n;
    handleError(clingo_model_symbols_size(model, clingo_show_type_shown, &n));
    trueSymbols.resize(n);
    handleError(clingo_model_symbols(model, clingo_show_type_shown, trueSymbols.data(), n));
    costProgram = program->preprocessedCost;
    for(const Literal* literal: *program->softLiterals){
        bool isTrue;
        handleError(clingo_model_is_true(model, literal->id, &isTrue));
        this->set(literal->id, isTrue);         // false ones too: a negative soft literal (not a) is read from its atom
        if(isTrue){
            trueSoftLiterals.push_back(literal);
            costProgram += program->weight(literal->id);
        }
    }

    if(program->readRestLiterals){              // TORC: the rest atoms from the model (unwritten atoms read false)
        for(const Literal* literal: *program->restLiterals){
            bool isTrue;
            handleError(clingo_model_is_true(model, literal->id, &isTrue));
            this->set(literal->id, isTrue);
        }
        ensuredRestLiterals = true;
    }
}

// the shown atoms true, without the text
void ClingoAssignment::ensureRestLiterals() const noexcept{
    if(ensuredRestLiterals) return;
    ensuredRestLiterals = true;
    ClingoAssignment* self = const_cast<ClingoAssignment*>(this);   // lazy: only the cached interpretation changes
    for(const clingo_symbol_t symbol: trueSymbols){
        const Literal* literal = program->literalBySymbol(symbol);
        if(literal != nullptr) self->set(literal->id, true);
    }
}

// { 'a', 'b'} from the symbols, with the names computed at grounding; the shown atoms are set true on the way
void ClingoAssignment::ensureOutputString() noexcept{
    if(ensuredOutputString) return;
    ensuredOutputString = true;
    const bool setRest = !ensuredRestLiterals;
    std::string out = "{ ";
    bool first = true;
    for(const clingo_symbol_t symbol: trueSymbols){
        const Literal* literal = program->literalBySymbol(symbol);
        // a shown term that is not an atom (#show t : body.): text only
        const std::string name = literal != nullptr ? literal->name : clingo::from_symbol_to_string(symbol);
        if(literal != nullptr && setRest) this->set(literal->id, true);
        if(avoidToShowRegex != nullptr && std::regex_match(name, *avoidToShowRegex)) continue;   // nullptr: all shown
        out += (first ? "'" : ", '") + name + "'";
        first = false;
    }
    outputString = out + "}";
    ensuredRestLiterals = true;
}

size_t Assignment::numberOfTrueLiterals(const Vector<const Literal*>* literals) const noexcept{
    size_t numberOfTL = 0;
    if(literals == nullptr || literals == this->program->softLiterals.get()) 
        return this->trueSoftLiterals.size();
    for(const Literal* l: *literals) {
        const bool& isT = this->isTrue(l);
        #ifdef __debug__ 
            LOG(LogLevel::DEBUG,"Is ",l," true?: ",isT);
        #endif
        if(isT) 
            ++numberOfTL;
    }
    return numberOfTL;
}

std::string Assignment::toOutputString(Vector<const Literal*> *toShow, const fiore::UMNWeightFunction* w) noexcept{
    if(this->isSolutionUnkown()) return outputString;
    ensureOutputString();
    std::ostringstream out;
    std::string prefix;
    if(result.isOptimum()){
        prefix = "Optimum ";
    }else if(result.isOptimumUnkownSat()){
        prefix = "Unkown ";
    }else if(result.isSat()){
        prefix = "";
    }else 
        assert(false);
    if(time != 0) out<<"Time "<<time<<" ";
    if(number == CONSTANTS::NONE32) number = AnswersetsPool::getInstance()->getNextIdAnswersetPrinted();
    if(!this->isSolutionUnkown()){
        // the cost only with a #minimize
        if(program->isOptimizationProblem) out<<"Cost: "<<this->cost()<<" ";
        out<<prefix<<"AnswerSet "<<number<<"° : "<<outputString;
    }
    else out<<outputString;
    return out.str();
};

std::string Assignment::toCostOutputString(Vector<const Literal*> *toShow) noexcept{
    if(this->isSolutionUnkown()) return outputString;
    std::ostringstream out;
    std::string prefix;
    if(result.isOptimum()){
        prefix = "Optimum ";
    }else if(result.isOptimumUnkownSat()){
        prefix = "Unkown ";
    }else if(result.isSat()){
        prefix = "";
    }else assert(false);
    if(time != 0) out<<"Time "<<time<<" ";
    if(number == CONSTANTS::NONE32) number = AnswersetsPool::getInstance()->getNextIdAnswersetPrinted();
    // out<<"Cost: "<<this->cost()<<" "<<prefix<<"AnswerSet "<<number<<"° : "<<this->toInterpretationString(false, true, toShow);
    out<<prefix<<"Cost AnswerSet "<<number<<"° : "<<this->cost();
    return out.str();
};

std::string Assignment::toString(bool printDefault)const noexcept{ return assignment->toInterpretationString(program->mapLiteralId2Literal.get());}
std::string Assignment::toAnswerSetString(const Vector<const Literal*> *toShow)const noexcept{return assignment->toAnswerSetString(program->mapLiteralId2Literal.get(), toShow);}
std::string Assignment::toInterpretationString(bool printUndefined, bool onlyTrueAtoms, Vector<const Literal*> *toShow){ return assignment->toInterpretationString(program->mapLiteralId2Literal.get(), printUndefined, onlyTrueAtoms, toShow);}
std::string Assignment::toConstraints()const noexcept{ return assignment->toConstraint(program->mapLiteralId2Literal.get());}

int64_t Assignment::cost(UMNWeightFunction* weight) noexcept{
    if(costProgram == CONSTANTS::INFPlus64 || weight == nullptr || weight == program->weightFunction.get()) 
        return costProgram;
    return program->preprocessedCost + sum(*weight, trueSoftLiterals);
}


std::vector<int32_t> Assignment::convertIntoAssumptions(const Vector<const Literal*>* literals) noexcept{
    const Vector<const Literal*>* lits = literals != nullptr ? literals : program->softLiterals.get();
    std::vector<int32_t> assumptions;
    for(const Literal* l: *lits) assumptions.push_back(this->isTrue(l->id)? l->id: not_(l->id));
    return assumptions;
}

void Assignment::foundNewAnswerset(std::string prefix, const std::string& verbose) noexcept{
    const int64_t &c = this->cost();
    int64_t &upperBound = AnswersetsPool::getInstance()->upperBound;
    if(c >= upperBound) return;
    upperBound = c;
    #ifndef NSTATS
        Statistics::getInstance()->addSeriesPoint("upperBound", c);
    #endif
    // std::string verbose = ArgParse::getInstance()->get(CONSTANTS::verboseKey);
    if(verbose == CONSTANTS::verboseLevel1) this->printCostAssignment(prefix);
    else if(verbose == CONSTANTS::verboseLevel2) this->printAssignment(prefix);
    this->setAsBestKnown();
}

void Assignment::printAssignment(std::string prefix) noexcept{
    const double& elasped = getWallTime();
    std::string elaspedStr = "Wall Time: " +std::to_string(elasped) + "s ";
    std::ostringstream out;
    // out<<prefix<<elaspedStr<<this->toInterpretationString(true, false);
    out<<prefix<<elaspedStr<<this->toOutputString();
    std::string outString = out.str();
    std::cout<<outString<<std::endl;
    #ifdef __debug__
        LOG(LogLevel::ALWAYS, outString);
    #endif

}

float Assignment::asymmetricInclusionDistance(const Assignment& other) noexcept{
    size_t thisNoInsideOther = 0;
    float thisSize = this->trueSoftLiterals.size();
    for(const Literal*& softLiteral: this->trueSoftLiterals){
        thisNoInsideOther += !other.isTrue(softLiteral);
    }
    return thisNoInsideOther / thisSize;
}

float Assignment::overlapDistance(const Assignment& other) noexcept{
    size_t minSize = std::min(this->trueSoftLiterals.size(), other.trueSoftLiterals.size());
    
    if (minSize == 0) {
        return 0.0f; 
    }

    size_t intersectionSize = 0;
    for(const Literal* const& softLiteral : this->trueSoftLiterals) {
        intersectionSize += other.isTrue(softLiteral);
    }

    return 1.0f - (static_cast<float>(intersectionSize) / minSize);
}



void Assignment::printCostAssignment(std::string prefix) noexcept{
    const double& elasped = getWallTime();
    std::string elaspedStr = "Wall Time: " +std::to_string(elasped) + "s ";
    std::ostringstream out;
    // out<<prefix<<elaspedStr<<this->toInterpretationString(true, false);
    out<<prefix<<elaspedStr<<this->toCostOutputString();
    std::cout<<out.str()<<std::endl;
    #ifdef __debug__
        LOG(LogLevel::ALWAYS, out.str());
    #endif

}

Assignment::~Assignment(){
    #ifdef __debug__
        int n = --AnswersetsPool::getInstance()->numberOfNotDeletedAnswerset;
        LOG(LogLevel::DEBUG, "Free answer sets: ",n);
    #endif
}

void Assignment::setAsBestKnown() noexcept{  AnswersetsPool::getInstance()->setBestKnownAnswerSet(this->id); }


inline AnswersetsPool* AnswersetsPool::instance = nullptr;
// inline size_t AnswersetsPool::autoIncId = 0;
// inline size_t AnswersetsPool::autoIncIdPrinted = 1;
AnswersetsPool* AnswersetsPool::getInstance() noexcept{ if(instance == nullptr) instance = new AnswersetsPool(); return instance; }

} // namespace fiore
