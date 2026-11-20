#pragma once
#include "../utils/utility.h"
#include "../clingo/utilityClingo.h"
#include "../clingo/stop.h"
#include "../utils/logger.h"


namespace fiore {

struct Program;
struct ClingoSolver;
struct Assignment;

struct Rule{
    bool choice;
    std::vector<uint32_t> head;
    std::vector<clingo_literal_t> body;

    virtual std::string toString(MapLiteralId2Literal* mapLiteralId2Literal = nullptr) const;
    Rule(bool choice, std::vector<uint32_t>&& head, std::vector<clingo_literal_t>&& body)noexcept: choice(choice), head(head), body(body){}
    virtual ~Rule() = default;
};

struct WeightRule: public Rule{
    int64_t lowerBound;
    WeightRule(bool choice, std::vector<uint32_t>&& head, std::vector<clingo_literal_t>&& body, const int64_t& lower_bound) noexcept: Rule(choice, std::move(head), std::move(body)), lowerBound(lower_bound){}
    std::string toString(MapLiteralId2Literal* mapLiteralId2Literal = nullptr) const override{ mapLiteralId2Literal->literal(CONSTANTS::NONE32); throw Args::BaseException("Aggregates not supported in this version"); return ""; };
};

struct PBConstraint{

    const Program* program;
    int64_t bound;
    UMNWeightFunction*  weightFunction;
    Vector<const Literal*>* ownedLiterals;
    int64_t preprocessedSum;

    PBConstraint(const Program* program, const int64_t bound = 0, Vector<const Literal*>* literals = nullptr, UMNWeightFunction*  weightFunction = nullptr) noexcept;
    
    inline int64_t weight(const fiore::Literal* l) const noexcept { return weightFunction->at(l->id); }
    inline void updateBound(const int64_t& bound)noexcept{ this->bound = bound; }
    inline virtual std::string toString() const noexcept{ std::ostringstream out; out<<"PBConstraint("<<ownedLiterals<<", "<<preprocessedSum<<", "<<bound<< ")"; return out.str();};

   virtual ~PBConstraint() = default;
} ;

std::ostream& operator<<(std::ostream& out, const PBConstraint& pb);

typedef std::vector<const Literal*>::iterator indexesLiterals;

struct Program{

    std::vector<std::unique_ptr<PBConstraint>> pBConstraints;
    std::vector<std::unique_ptr<Rule>> rules;
    std::unique_ptr<MapLiteralId2Literal> mapLiteralId2Literal;
    std::unique_ptr<UMNWeightFunction> weightFunction;
    std::unique_ptr<Vector<const Literal*>> softLiterals;
    std::unique_ptr<Vector<const Literal*>> restLiterals;
    std::unordered_map<std::string, const Literal*> mapLiteralName2Literal;
    std::unordered_map<clingo_symbol_t, const Literal*> mapLiteralSymbol2Literal;   // filled at grounding
    std::unordered_set<clingo_atom_t> softAtomIds;      // atoms of the soft literals (after preprocessCost)
    bool readRestLiterals = false;                      // TORC: ClingoAssignment reads the rest atoms of every model
    uint32_t maxSoftLiteralId;
    uint32_t maxOriginLiteralId;
    uint32_t maxLiteralId;
    std::unordered_set<int64_t> weights;
    std::unique_ptr<PerfectNHash<clingo_literal_t, clingo_literal_t>> mapTarget2External;
    const int64_t ZEROCOST = 0;
    inline bool orderSoftLiterals(const Literal* a, const Literal* b) const noexcept{ return byDescendingWeight(a,b); }
    bool byDescendingWeight(const Literal* a, const Literal* b) const noexcept;
    inline bool isWeighted() const noexcept{ return weights.size() > 1; }
    inline bool isUnWeighted() const noexcept{ return weights.size() == 1; }
    inline const Literal* literalBySymbol(clingo_symbol_t symbol) const noexcept{ auto it = mapLiteralSymbol2Literal.find(symbol); return it != mapLiteralSymbol2Literal.end() ? it->second : nullptr; }
    // a soft literal's atom: both polarities are written by ClingoAssignment::init
    inline bool isSoft(clingo_literal_t literalId) const noexcept{ return softAtomIds.count(std::abs(literalId)) > 0; }

    // Clingo
    clingo_control* ctl;
    clingo_backend_t * bk;
        
    // negative weights are normalized (w·l = w + |w|·¬l), both polarities of an atom merged, zero weights dropped
    static int64_t preprocessCost(Vector<const Literal*>&  softLiterals, UMNWeightFunction& weightFunction, MapLiteralId2Literal* mapLiteralId2Literal)noexcept;
    static std::vector<std::pair<const Literal*, const Literal*>> bothPolarityLiterals(Vector<const Literal*>& softLiterals, MapLiteralId2Literal* mapLiteralId2Literal)noexcept;
    static std::unordered_map<clingo_atom_t, std::vector<const Literal*>> groupById(Vector<const Literal*>& literals)noexcept;

    int64_t preprocessedCost;
    void addRule(Rule* &&rule, bool adding2Backend = false) noexcept;
    Literal* freshLiteral() noexcept;
    Literal* externalLiteral(clingo_external_type_e type = clingo_external_type_false) noexcept;
    // the backend of ctl: bk is non-null only while open (it belongs to ctl, never freed here)
    void openBackend() noexcept { if(bk != nullptr) return; handleError(clingo_control_backend(ctl, &bk)); handleError(clingo_backend_begin(bk)); }
    void closeBackend() noexcept { if(bk == nullptr) return; handleError(clingo_backend_end(bk)); bk = nullptr; }
    PBConstraint* addPBConstraint(int64_t bound = 0, Vector<const Literal*>* &&literals = nullptr, UMNWeightFunction* weightFunction = nullptr)noexcept;
    std::string literalIdsToString(const std::vector<clingo_literal_t> literalIds) noexcept;
    inline const int64_t& weight(const clingo_literal_t& literalId) const {return (uint32_t)std::abs(literalId) > maxSoftLiteralId ? ZEROCOST : weightFunction->at(literalId);}
    inline std::string weightsToString(bool useLiteralIds = false) noexcept{ return weightFunction->toString(!useLiteralIds ? mapLiteralId2Literal.get() : nullptr); }
    inline std::string toString() noexcept{ std::vector<std::string> res; for(const std::unique_ptr<Rule>& r: rules) res.push_back(r->toString(mapLiteralId2Literal.get())); return join("\n", std::move(std::move(res)));}
    std::vector<Cluster> partition(size_t minSize = CONSTANTS::INFPlus32, size_t m = CONSTANTS::NONE64, Vector<const Literal*>* targetLiterals = nullptr)noexcept;
    std::pair<Vector<const Literal*>, std::vector<size_t>>  flatPartition(size_t minSize = CONSTANTS::INFPlus32, size_t m = CONSTANTS::NONE64, Vector<const Literal*>* targetLiterals = nullptr)noexcept;
    UMNWeightFunction* createWeightsPartitioned(const std::vector<Cluster>& clusters, int64_t (*functionRepresentativeWeight)(const Cluster&, UMNWeightFunction*))noexcept;
    inline const Literal* literalByName(std::string name) const noexcept{ auto it = mapLiteralName2Literal.find(name); return it != mapLiteralName2Literal.end() ? it->second : nullptr;}
    bool isOptimizationProblem;

    virtual ~Program(){
        if(ctl != nullptr){
            // no longer interrupted by the signal handler (only if it is this program's control)
            clingo_control_t* own = ctl;
            clingo::controlToInterrupt.compare_exchange_strong(own, nullptr);
            clingo_control_free(ctl);
        }
    }

    friend ClingoSolver;
    void setPolaritiesLocalSearch(const Assignment* ass = nullptr, Vector<const Literal *>* literals = nullptr) noexcept;
    void setPolarityLocalSearch(const Literal* l, bool polarity) noexcept { setPolarityLocalSearch(l->id, polarity); }
    void setPolarityLocalSearch(const clingo_literal_t l, bool polarity) noexcept;
    // pin: the external of a soft literal set true, so its sign heuristic (above the default polarity) applies
    void pinLocalSearch(const clingo_literal_t l) noexcept;
    void unpinLocalSearch(const clingo_literal_t l) noexcept;
    // a new atom true iff one of `literals` is (one rule per literal), added through the backend after grounding
    const fiore::Literal* addDisjunction(const std::vector<const fiore::Literal*>& literals, const std::string& name) noexcept;
    // TOP/BOTTOM of the program literals: an atom reserved in clingo (CONSTANTS::TOP/BOTTOM are the solver ones)
    inline clingo_literal_t TOP() const noexcept { return top; }
    inline clingo_literal_t BOTTOM() const noexcept { return bottom; }
protected:
    size_t priorityPolarity;
    bool negativeWeights;
    
    struct ProgramGrounder{

        ProgramGrounder();

        void addArgControl(std::string key, std::string value = std::string()) noexcept;

        // def rule(self, choice: bool, head: List[int], body: List[int]) -> None:
        //     """
        //     Observe rules passed to the solver.

        //     Parameters
        //     ----------
        //     choice : bool
        //         Determines if the head is a choice or a disjunction.
        //     head : List[int]
        //         List of program atoms forming the rule head.
        //     body : List[int]
        //         List of program literals forming the rule body.

        //     Returns
        //     -------
        //     None
        //     """
        // static inline bool rule(bool choice, clingo_atom_t const *head, size_t head_size, clingo_literal_t const *body, size_t body_size, void *data){ 
        //     Program::ProgramGrounder* g = (Program::ProgramGrounder*)data;
        //     if(!g->first) return true;
        //     // program->mapLiteralId2Literal
        //     // program->maxOriginLiteralId
        //     // g->program->addRule(new Rule(choice, std::vector<clingo_atom_t>(head, head + head_size), std::vector<clingo_literal_t>(body, body + body_size))); 
        //     return true;
        // }


        // def weight_rule(self, choice: bool, head: List[int], lower_bound: int,
        //                 body: List[Tuple[int,int]]) -> None:
        //     """
        //     Observe rules with one weight constraint in the body passed to the
        //     solver.

        //     Parameters
        //     ----------
        //     choice : bool
        //         Determines if the head is a choice or a disjunction.
        //     head : List[int]
        //         List of program atoms forming the head of the rule.
        //     lower_bound:
        //         The lower bound of the weight constraint in the rule body.
        //     body : List[Tuple[int,int]]
        //         List of weighted literals (pairs of literal and weight) forming the
        //         elements of the weight constraint.

        //     Returns
        //     -------
        //     None
        //     """
        // bool weight_rule(bool choice, clingo_atom_t const *head, size_t head_size, clingo_weight_t lower_bound, clingo_weighted_literal_t const *body, size_t body_size, void *data){
        //     throw Args::BaseException("I should not have aggregates in my experiments");
        // }


        // def minimize(self, priority: int, literals: List[Tuple[int,int]]) -> None:
        //     """
        //     Observe minimize directives (or weak constraints) passed to the
        //     solver.

        //     Parameters
        //     ----------
        //     priority : int
        //         The priority of the directive.
        //     literals : List[Tuple[int,int]]
        //         List of weighted literals whose sum to minimize (pairs of literal
        //         and weight).

        //     Returns
        //     -------
        //     None
        //     """
        //     # For now we are ignoring priority
        static inline bool minimize(clingo_weight_t priority, clingo_weighted_literal_t const *literals, size_t size, void *data){
            Program::ProgramGrounder* g = (Program::ProgramGrounder*)data;
            if(!g->first) return true;
            #ifdef __debug__
                LOG(LogLevel::INFO_1," Minimize Callback size soft: ",size);
            #endif 
            g->program->maxSoftLiteralId = 0;
            g->program->negativeWeights = false;
            for(size_t i = 0; i < size; ++i){
                const clingo_literal_t &literalId = literals[i].literal;
                const clingo_atom_t &atomId = std::abs(literals[i].literal);
                #ifdef __debug__
                    LOG(LogLevel::DEBUG,"Adding soft literal: ",literalId, " with weight: ",literals[i].weight);
                #endif
                if(g->program->maxSoftLiteralId < atomId) g->program->maxSoftLiteralId = atomId;
                const clingo_weight_t &w =  literals[i].weight;
                g->program->weights.emplace(w);
                g->program->negativeWeights |= w < 0;          // normalized by preprocessCost
                g->weightDict[literalId] += w;
            }
            #ifdef __debug__
                LOG(LogLevel::DEBUG," weightDictSize: ",g->weightDict.size());
                LOG(LogLevel::DEBUG," weightDict: ",g->weightDict);
            #endif
            g->program->isOptimizationProblem = size > 0;
            return true;
        }
        virtual Program* ground(const std::vector<std::string>& ASPInputs);
        virtual ~ProgramGrounder() = default;
        
    protected:
        // the program filled by ground(): derived grounders return a derived Program
        virtual Program* createProgram() const { return new Program(); }
        
       
    private: 
        Program* program;
        std::vector<std::string> arguments;
        std::unordered_map<clingo_literal_t, clingo_weight_t> weightDict;
        bool first;
    };
    Program();
    void backendAddRule(const Rule* rule) noexcept;
    clingo_literal_t top;
    clingo_literal_t bottom;
};


std::ostream& operator<<(std::ostream& out, Program& prog);

} // namespace fiore
