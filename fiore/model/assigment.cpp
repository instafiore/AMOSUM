#include "assigment.h"
#include "program.h"
#include "../utils/stats.h"


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


ClingoAssignment::ClingoAssignment(Program* program, const size_t number, const clingo_model_t* model, const AssignmentType type): Assignment(program, number, type) {  
    this->init(model);
}

ClingoAssignment::ClingoAssignment(Program* program, const clingo_model_t* model, const AssignmentType type): Assignment(program, CONSTANTS::NONE32, type) {  
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


void ClingoAssignment::init(const clingo_model_t* model){
    
    clingo_symbol_t *atoms = NULL;
    size_t atoms_n;
    clingo_symbol_t const *it, *ie;
    char *name = NULL;
    size_t str_n = 0;

    // handleError(clingo_model_symbols_size(model, clingo_show_type_all, &atoms_n));
    handleError(clingo_model_symbols_size(model, clingo_show_type_shown, &atoms_n));

    if (!(atoms = (clingo_symbol_t*)malloc(sizeof(*atoms) * atoms_n))) {
        clingo_set_error(clingo_error_bad_alloc, "could not allocate memory for atoms");
        #ifdef __debug__
            LOG(LogLevel::CLINGO_ERROR, "could not allocate memory for atoms");
        #endif
    }

    // handleError(clingo_model_symbols(model, clingo_show_type_all, atoms, atoms_n));
    handleError(clingo_model_symbols(model, clingo_show_type_shown, atoms, atoms_n));

    outputString = "{ " ;

    for (it = atoms, ie = atoms + atoms_n; it != ie; ++it) {
        size_t n;
        char *str_new;
   
        // determine size of the string representation of the next symbol in the model
        handleError(clingo_symbol_to_string_size(*it, &n));
   
        if (str_n < n) {
            // allocate required memory to hold the symbol's string
            if (!(str_new = (char*)realloc(name, sizeof(*name) * n))) {
                clingo_set_error(clingo_error_bad_alloc, "could not allocate memory for symbol's string");
                #ifdef __debug__
                    LOG(LogLevel::CLINGO_ERROR, "could not allocate memory for symbol's string");
                #endif
            }
            name = str_new;
            str_n = n;
        }
   
        // retrieve the symbol's string
        handleError(clingo_symbol_to_string(*it, name, n));
        const Literal* literal = program->literalByName(name);

        #ifdef __debug__
            LOG(LogLevel::DEBUG, "Setting ",literal, " to true");
        #endif

        this->set(literal->id, true);
        outputString += std::string("\'") +std::string(name)+ std::string("\'") + (it + 1 != ie ? std::string(", ") : std::string(""));
    }

    outputString += "}";


    //  assuming one level
    size_t cs;
    handleError(clingo_model_cost_size(model, &cs));
    clingoCost = (int64_t*) malloc(sizeof(int64_t) * cs);
    handleError(clingo_model_cost(model, clingoCost, cs));

    // TODO: maybe trying to optimize it having two lists: one for the original soft literals another for the new soft literals     
    costProgram = program->preprocessedCost;
    #ifdef __debug__
        LOG(LogLevel::DEBUG," Program soft literals size: ", program->softLiterals->size());
    #endif
    for(const Literal* literal: *program->softLiterals){
        bool isTrue;
        handleError(clingo_model_is_true(model, literal->id, &isTrue));
        if(isTrue){
            this->set(literal->id, true);
            trueSoftLiterals.push_back(literal);
            const int64_t& w = program->weight(literal->id);
            costProgram+= w;
            // if(w < 0)
            //     NOOP;
        }
    }


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

std::string Assignment::toOutputString(Vector<const Literal*> *toShow) noexcept{
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
    // out<<"Cost: "<<this->cost()<<" "<<prefix<<"AnswerSet "<<number<<"° : "<<this->toInterpretationString(false, true, toShow);
    if(!this->isSolutionUnkown()) out<<"Cost: "<<this->cost()<<" "<<prefix<<"AnswerSet "<<number<<"° : "<<outputString;
    else out<<outputString;
    return out.str();
};

std::string Assignment::toCostOutputString(Vector<const Literal*> *toShow) noexcept{
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
    this->setAsCurrentOptimum();
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

void Assignment::setAsCurrentOptimum() noexcept{  AnswersetsPool::getInstance()->setOptimalAnswerSet(this->id); }


inline AnswersetsPool* AnswersetsPool::instance = nullptr;
// inline size_t AnswersetsPool::autoIncId = 0;
// inline size_t AnswersetsPool::autoIncIdPrinted = 1;
AnswersetsPool* AnswersetsPool::getInstance() noexcept{ if(instance == nullptr) instance = new AnswersetsPool(); return instance; }

} // namespace fiore
