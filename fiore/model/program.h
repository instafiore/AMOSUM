#pragma once
#include "../utils/utility.h"
#include "../clingo/utilityClingo.h"
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
    Vector<const Literal*>* literals;
    int64_t preprocessedSum;

    PBConstraint(const Program* program, const int64_t bound = 0, Vector<const Literal*>* literals = nullptr, UMNWeightFunction*  weightFunction = nullptr) noexcept;
    
    inline void updateBound(const int64_t& bound)noexcept{ this->bound = bound; }
    inline virtual std::string toString() const noexcept{ std::ostringstream out; out<<"PBConstraint("<<literals<<", "<<preprocessedSum<<", "<<bound<< ")"; return out.str();};

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
    uint32_t maxSoftLiteralId;
    uint32_t maxOriginLiteralId;
    uint32_t maxLiteralId;
    std::unordered_set<int64_t> weights;
    inline bool orderSoftLiterals(const Literal* a, const Literal* b) const noexcept{ return byDescendingWeight(a,b); }
    bool byDescendingWeight(const Literal* a, const Literal* b) const noexcept;
    inline bool isWeighted() const noexcept{ return weights.size() > 1; }
    inline bool isUnWeighted() const noexcept{ return weights.size() == 1; }

    // Clingo
    clingo_control* ctl;
    clingo_backend_t * bk;
        
    static int64_t preprocessCost(Vector<const Literal*>&  softLiterals, UMNWeightFunction& weightFunction, MapLiteralId2Literal* mapLiteralId2Literal, bool update = true)noexcept;
    static std::vector<std::pair<const Literal*, const Literal*>> bothPolarityLiterals(Vector<const Literal*>& softLiterals, MapLiteralId2Literal* mapLiteralId2Literal)noexcept;
    
    int64_t preprocessedCost;
    void addRule(Rule* &&rule, bool adding2Backend = false) noexcept;
    Literal* freshLiteral() noexcept;
    PBConstraint* addPBConstraint(int64_t bound = 0, Vector<const Literal*>* &&literals = nullptr, UMNWeightFunction* weightFunction = nullptr)noexcept;
    std::string literalIdsToString(const std::vector<clingo_literal_t> literalIds) noexcept;
    inline const int64_t& weight(const clingo_literal_t& literalId) const {return weightFunction->at(literalId);}
    inline std::string weightsToString(bool useLiteralIds = false) noexcept{ return weightFunction->toString(!useLiteralIds ? mapLiteralId2Literal.get() : nullptr); }
    inline std::string toString() noexcept{ std::vector<std::string> res; for(const std::unique_ptr<Rule>& r: rules) res.push_back(r->toString(mapLiteralId2Literal.get())); return join("\n", std::move(std::move(res)));}
    std::vector<Cluster> partition(size_t minSize = CONSTANTS::INFPlus32, size_t m = CONSTANTS::NONE64, Vector<const Literal*>* targetLiterals = nullptr)noexcept;
    std::pair<Vector<const Literal*>, std::vector<size_t>>  flatPartition(size_t minSize = CONSTANTS::INFPlus32, size_t m = CONSTANTS::NONE64, Vector<const Literal*>* targetLiterals = nullptr)noexcept;
    UMNWeightFunction* createWeightsPartitioned(const std::vector<Cluster>& clusters, int64_t (*functionRepresentativeWeight)(const Cluster&, UMNWeightFunction*))noexcept;
    inline const Literal* literalByName(std::string name) const noexcept{ auto it = mapLiteralName2Literal.find(name); return it != mapLiteralName2Literal.end() ? it->second : nullptr;}
    bool isOptimizationProblem;

    ~Program(){
        if(bk != nullptr) free(bk);
        if(ctl != nullptr) clingo_control_free(ctl);
    }

    friend ClingoSolver;
    void setPolaritiesLocalSearch(const Assignment* ass = nullptr, size_t priority = CONSTANTS::INFPlus32, Vector<const Literal *>* literals = nullptr) noexcept;
protected:
    size_t priorityPolarity;
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
            for(size_t i = 0; i < size; ++i){
                const clingo_literal_t &literalId = literals[i].literal;
                const clingo_atom_t &atomId = std::abs(literals[i].literal);
                #ifdef __debug__
                    LOG(LogLevel::DEBUG,"Adding soft literal: ",literalId, " with weight: ",literals[i].weight);
                #endif
                auto it =  g->weightDict.find(literalId);
                if(g->program->maxSoftLiteralId < atomId) g->program->maxSoftLiteralId = atomId;
                const clingo_weight_t &w =  literals[i].weight;
                g->program->weights.emplace(w);
                if(w < 0){
                    // In case this benchmark has negative weights we stop the run!
                    setExitCode(1, true, "No negative weights supported");
                }
                it == g->weightDict.end() ? g->weightDict[literals[i].literal] = w : it->second += w;
            }
            #ifdef __debug__
                LOG(LogLevel::DEBUG," weightDictSize: ",g->weightDict.size());
                LOG(LogLevel::DEBUG," weightDict: ",g->weightDict);
            #endif
            g->program->isOptimizationProblem = size > 0;
            return true;
        }
        virtual Program* ground(const std::vector<std::string>& ASPInputs);
        
    private: 
        Program* program;
        std::vector<std::string> arguments;
        std::unordered_map<clingo_literal_t, clingo_weight_t> weightDict;
        bool first;
    };
    Program();
    void backendAddRule(const Rule* rule) noexcept;
};


std::ostream& operator<<(std::ostream& out, Program& prog);

} // namespace fiore
