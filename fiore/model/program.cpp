#include "program.h"
#include "assigment.h"


namespace fiore {

std::ostream& operator<<(std::ostream& out, const PBConstraint& pb){ return out<<pb.toString();}
std::ostream& operator<<(std::ostream& out, Program& prog){ return out<<prog.toString();}

std::string Rule::toString(MapLiteralId2Literal* mapLiteralId2Literal) const{
    std::ostringstream o;
    const bool withNames = mapLiteralId2Literal != nullptr;
    if(this->choice) o<<'{';
    for(size_t i = 0; i < this->head.size(); ++i){
        std::string delim = this->choice ? ";" : "|";
        std::ostringstream oss;
        withNames ? oss<<mapLiteralId2Literal->literal(head[i]) : oss<<this->head[i];
        o<<oss.str()<<(i < this->head.size() - 1 ? delim : std::string(""));
    }
    if(this->choice) o<<'}';

    if(this->body.size() > 0) o<<std::string(":- ");
    for(size_t i = 0; i < this->body.size(); ++i){
        std::ostringstream oss;
        withNames ? oss<<mapLiteralId2Literal->literal(body[i]) : oss<<this->body[i];
        o<<oss.str()<<(i < this->body.size() - 1 ? std::string(", ") : std::string(""));
    }
    o<<'.';
    return o.str();
}

std::ostream& operator<<(std::ostream& o, const Rule& rule){ return o<<rule.toString();}
std::ostream& operator<<(std::ostream& o, const Rule* rule){ return o<<*rule; }


PBConstraint::PBConstraint(const Program* program, const int64_t upperBound, Vector<const Literal*>* softLiterals, UMNWeightFunction*  weightFunction)noexcept:
    program(program), upperBound(upperBound),
    weightFunction(weightFunction == nullptr ? program->weightFunction.get() : weightFunction), 
    softLiterals(softLiterals == nullptr ? program->softLiterals.get() : softLiterals){
    // this->softLiterals->sort([](const Literal* a, const Literal* b){ return a->id < b->id; });
    preprocessedCost = softLiterals != nullptr || weightFunction != nullptr ? Program::preprocessCost(*this->softLiterals, *this->weightFunction, program->mapLiteralId2Literal.get()) : program->preprocessedCost;
    #ifdef __debug__
        LOG(LogLevel::DEBUG_1, "Created ",this," - PBConstraint::init");
    #endif
}

Program::Program(): restLiterals(new Vector<const Literal*>), isOptimizationProblem(false), priorityPolarity(1){}

void Program::addRule(Rule* &&rule, bool adding2Backend) noexcept{
    #ifdef __debug__
        LOG(LogLevel::DEBUG, "adding rule:  ",rule, " - Program::addRule");
    #endif
    // this->rules.push_back(rule);
    this->rules.emplace_back(std::move(rule));
    if(adding2Backend) backendAddRule(rule);
}

Literal* Program::freshLiteral() noexcept{
    handleError(clingo_backend_begin(bk));
        clingo_atom_t id;
        handleError(clingo_backend_add_atom(bk,NULL,&id));
    handleError(clingo_backend_end(bk));
    this->addRule(new Rule(true, std::vector<clingo_atom_t>{id},  std::vector<clingo_literal_t>()), true);
    std::string name = createLiteralString(id);
    Literal* newLiteral = new Literal(id, name);
    mapLiteralId2Literal->addFreshLiteral(newLiteral);
    return newLiteral;
}


PBConstraint* Program::addPBConstraint(int64_t upperBound, 
    Vector<const Literal*>*  &&softLiterals, UMNWeightFunction* weightFunction)noexcept{
    PBConstraint* pbConstraint = new PBConstraint(this, upperBound, std::move(softLiterals), weightFunction);
    pBConstraints.emplace_back(pbConstraint);
    return pbConstraint;
}

std::string Program::literalIdsToString(const std::vector<clingo_literal_t> literalIds) noexcept{
    std::vector<const Literal*> res;
    for(const clingo_literal_t& literalId: literalIds) res.push_back(mapLiteralId2Literal->literal(literalId));
    std::ostringstream o;
    o<<res;
    return o.str();
}

int64_t Program::preprocessCost(Vector<const Literal*>&  softLiterals, 
    UMNWeightFunction& weightFunction, MapLiteralId2Literal* mapLiteralId2Literal, bool update)noexcept{
    std::vector<std::pair<const Literal*, const Literal*>> bothPolarityLiteralVec = Program::bothPolarityLiterals(softLiterals, mapLiteralId2Literal);
    int64_t preprocessedCost = 0;
    for(const std::pair<const Literal*, const Literal*>& literals: bothPolarityLiteralVec){
        std::vector<int64_t> weights;
        weights.push_back(weightFunction.at(literals.first->id));
        weights.push_back(weightFunction.at(literals.second->id));
        int64_t minWeight = *std::min_element(weights.begin(), weights.end());
        if(update){
            for(const Literal* literal: std::vector<const Literal*>{literals.first, literals.second}){
                int64_t newWeight = weightFunction.at(literal->id) - minWeight;
                if(newWeight == 0) softLiterals.remove(literal);
                weightFunction.set(literal->id, newWeight);
            }
        }
        preprocessedCost += minWeight;
    }
    #ifdef __debug__
        LOG(LogLevel::INFO, "preprocessedCost: ", preprocessedCost, " - Program::preprocessCost");
    #endif
    return preprocessedCost;
}


std::vector<std::pair<const Literal*, const Literal*>>  Program::bothPolarityLiterals(Vector<const Literal*>&  softLiterals, MapLiteralId2Literal* mapLiteralId2Literal)noexcept{
    std::vector<std::pair<const Literal*, const Literal*>> bothPolarityLiterals;
    for(const Literal* literal: softLiterals){
        const Literal* negatedLiteral = mapLiteralId2Literal->literal(not_(literal->id));
        if(literal->positive() && softLiterals.contains(negatedLiteral))
            bothPolarityLiterals.push_back(std::make_pair<>(literal, negatedLiteral));
    }  
    return bothPolarityLiterals;
}


std::vector<Cluster>  Program::partition(size_t minSize, size_t m, Vector<const Literal*>* targetLiterals) noexcept{
    std::unordered_set<int64_t> weights;
    std::unordered_map<int64_t, size_t> weightsOcc;
    const Vector<const Literal*> &softLits = targetLiterals != nullptr ? *targetLiterals : *this->softLiterals.get();
    for(const Literal* l: softLits){ 
        const int64_t& w = weightFunction->at(l->id);
        weights.emplace(w);
        auto it = weightsOcc.find(w);
        it == weightsOcc.end() ? weightsOcc[w] = 1 : it->second += 1;
    }
    if(m == CONSTANTS::NONE32 || m > weights.size()) m = weights.size();
    size_t n = softLits.size();
    if(n == 0) return std::vector<Cluster>();
    std::vector<std::pair<size_t, int64_t> > diff; 
    for(size_t i = 0; i < n-1; ++i){
        const Literal* li = softLits.get(i);
        const Literal* lj = softLits.get(i+1);
        int64_t d =  weightFunction->at(li->id) - weightFunction->at(lj->id);
        if(d == 0) continue;
        std::pair<size_t, int64_t> item{i+1, d};
        diff.push_back(item);
    }

    assert(diff.size() + 1 == weights.size());

    #ifdef __debug__
        LOG(LogLevel::INFO_1, "weights: ", weights);
        LOG(LogLevel::INFO_1, "weightsOcc: ", weightsOcc);
        LOG(LogLevel::INFO_1, "weightFunction: ", weightFunction->toString(this->mapLiteralId2Literal.get(), false));
        LOG(LogLevel::INFO_1, "softLits: ", softLits);
        LOG(LogLevel::INFO_1, "before diff: ", diff);
    #endif

    std::sort(diff.begin(), diff.end(), [](std::pair<size_t, int64_t>& a, std::pair<size_t, int64_t>& b){return a.second > b.second;});
    LOG(LogLevel::INFO_1, "after diff: ", diff);
    size_t cuts = m-1;
    if(cuts < diff.size()) sliceVecInPlace(diff, 0, cuts);
    std::vector<size_t> indexes;
    for(size_t i = 0; i < diff.size(); ++i) indexes.push_back(diff[i].first);
    indexes.push_back(n);
    std::sort(indexes.begin(), indexes.end());
    std::vector<Cluster> clusters;
    size_t iStart = 0;
    for(size_t i = 0; i < m; ++i){
        size_t iEnd = indexes[i];
        clusters.push_back(Cluster(softLits.slice(iStart, iEnd)));
        iStart = iEnd;
    }

    if(minSize == CONSTANTS::INFPlus32) return clusters;

    std::vector<Cluster> mergedClusters;
    for(size_t i = 0; i < m; ++i){
        Vector<const Literal*>& lits = clusters[i].literals;
        while(lits.size() < minSize && i + 1 < m){
            ++i;
            for(const Literal* l: clusters[i].literals) lits.push_back(l);
        }
        mergedClusters.emplace_back(std::move(lits));
    }

    #ifdef __debug__
        double meanSize = ((double)n)/ m;
        LOG(LogLevel::INFO_1, "Clusters: ",clusters);
        LOG(LogLevel::INFO_1, "#Clusters: ",m, " mean size: ",meanSize, " number of soft: ",n);
    #endif
    return mergedClusters;
}


std::pair<Vector<const Literal*>, std::vector<size_t>>  Program::flatPartition(size_t minSize, size_t m, Vector<const Literal*>* targetLiterals) noexcept{
    Vector<const Literal*>* targets = targetLiterals != nullptr ? targetLiterals : this->softLiterals.get();
    std::vector<size_t> indexes;
    Vector<const Literal*> flatTargets;
    std::vector<Cluster> clusters = this->partition(minSize, m, targets);
    

    size_t it = 0;
    indexes.push_back(it);
    for(const Cluster& c: clusters){ 
        const Vector<const Literal*>& clusterLiterals = c.literals;
        for(const Literal* l: clusterLiterals){
            flatTargets.push_back(l);
            ++it;
        }
        indexes.push_back(it);
    }
    assert(it == flatTargets.size());

    #ifdef __debug__
        std::vector<const Literal*> splittingLiterals;
        for(size_t i = 0; i < indexes.size()-1; ++i)
            splittingLiterals.push_back(flatTargets.get(indexes[i]));
        LOG(LogLevel::INFO_1, "Shuffling indexes ",indexes);
        LOG(LogLevel::INFO_1,"Splitting Literals: ", splittingLiterals);
    #endif

    return std::make_pair<>(flatTargets, indexes);
}

UMNWeightFunction* Program::createWeightsPartitioned(const std::vector<Cluster>& clusters, int64_t (*functionRepresentativeWeight)(const Cluster&, UMNWeightFunction*))noexcept{
    UMNWeightFunction* weightFunctionPartitioned = new UMNWeightFunction(this->maxSoftLiteralId + 1);
    for(const Cluster &cluster: clusters){
        int64_t representativeWeight = functionRepresentativeWeight(cluster, this->weightFunction.get());
        for(const Literal* literal: cluster.literals)
            weightFunctionPartitioned->set(literal->id, representativeWeight);
    }
    return weightFunctionPartitioned;
}

void Program::ProgramGrounder::addArgControl(std::string key, std::string value) noexcept{
    std::string valueStr = value.size() != 0 ? "="+value : "";
    std::string arg = "--"+key+valueStr;
    this->arguments.push_back(arg);
}

Program::ProgramGrounder::ProgramGrounder(): program(new Program()), first(true){}

Program* Program::ProgramGrounder::ground(std::vector<std::string> ASPInputs){
    if(!first) throw Args::BaseException("Grounder already used, pleace create another one");
    

    this->addArgControl("opt-mode", "ignore");
    this->addArgControl("heuristic", "Domain");
    #ifdef __debug__
        LOG(LogLevel::DEBUG, "argumentsGrounder: ", this->arguments, " - Program::ProgramGrounder::ground()");
        auto startLoadingTime = startTimer();
    #endif

    program->mapLiteralId2Literal.reset(new MapLiteralId2Literal());


    handleError(clingo_control_new(fromVecStr2VecCstring(arguments), arguments.size(), clingo::clingoLogger, NULL, CONSTANTS::INFPlus32, &program->ctl));
    for(std::string aspInput: ASPInputs) {
         #ifdef __debug__
            auto startLoadingTimeASPInput = startTimer();
        #endif
        clingo_control_load(program->ctl, aspInput.c_str());
        #ifdef __debug__
            auto endLoadingTimeASPInput = elapsedTime(startLoadingTimeASPInput);
            LOG(LogLevel::INFO_1,"Loading time for asp input ",aspInput,": ",endLoadingTimeASPInput);
        #endif
    }
    
    clingo_ground_program_observer obs{ .minimize = minimize};
    handleError(clingo_control_register_observer(program->ctl, &obs, false, this));
    clingo_part_t parts[] = {{"base", NULL, 0}};

    clingo_configuration_t *conf;
    clingo_control_configuration(program->ctl, &conf);

    #ifdef __debug__
            auto endLoadingTime = elapsedTime(startLoadingTime);
            LOG(LogLevel::INFO_1,"Loading time: ",endLoadingTime);
    #endif

    #ifdef __debug__
        auto startGroundingtime = startTimer();
    #endif
    handleError(clingo_control_ground(program->ctl, parts, 1, NULL, NULL));

    #ifdef __debug__
        auto endGroundingtime = elapsedTime(startGroundingtime);
        LOG(LogLevel::INFO_1, "Grounding time: ", endGroundingtime);
    #endif

    std::unordered_map<clingo_literal_t, const Literal*> mapLiteralId2Literal;

    clingo_symbolic_atoms_t const *symbolic_atoms;
    clingo_symbolic_atom_iterator_t symbolic_atoms_it, symbolic_atoms_ie;

    handleError(clingo_control_symbolic_atoms(program->ctl, &symbolic_atoms));
    handleError(clingo_symbolic_atoms_end(symbolic_atoms, &symbolic_atoms_ie));
    handleError(clingo_symbolic_atoms_begin(symbolic_atoms, NULL, &symbolic_atoms_it));
    
    #ifdef __debug__
        auto startCreationgLiteralsTime = startTimer();
    #endif

    uint32_t &maxLiteralId = program->maxOriginLiteralId;
    maxLiteralId = 0;
    
    while(true){
        bool equal;
        clingo_literal_t literalId;
        clingo_symbol_t name;

        // stop iteration if the end is reached
        handleError(clingo_symbolic_atoms_iterator_is_equal_to(symbolic_atoms, symbolic_atoms_it, symbolic_atoms_ie, &equal));
        if (equal) { break; }

        handleError(clingo_symbolic_atoms_symbol(symbolic_atoms, symbolic_atoms_it, &name));
        std::string nameStr = clingo::from_symbol_to_string(name) ;

        handleError(clingo_symbolic_atoms_literal(symbolic_atoms, symbolic_atoms_it, &literalId));
        const Literal* literal = new Literal(literalId, nameStr);
        #ifdef __debug__
            LOG(LogLevel::DEBUG, "Literal: ", literal);
        #endif
        // mapLiteralId2Literal[literalId] = literal;
        program->mapLiteralId2Literal->set(literalId, literal);
        program->mapLiteralName2Literal[nameStr] = literal;
        if((weightDict.find(literal->id) == weightDict.end()) && (weightDict.find(not_(literal->id)) == weightDict.end()))
            program->restLiterals->push_back(literal);

        clingo_symbolic_atoms_next(symbolic_atoms, symbolic_atoms_it, &symbolic_atoms_it);

        uint32_t id = std::abs(literal->id);
        if(id > maxLiteralId) maxLiteralId = literal->id;
    }

    program->maxLiteralId = std::max(maxLiteralId, program->maxSoftLiteralId);

    #ifdef __debug__
        auto endCreationgLiteralTime = elapsedTime(startCreationgLiteralsTime);
        LOG(LogLevel::INFO_1, "Creation Literals time: ", endCreationgLiteralTime);
    #endif


    #ifdef __debug__
        auto startCreationgMapLiteralId2LiteralTime = startTimer();
        double meanTimeIteration = 0;
        size_t iterationN = 0;
    #endif

    #ifdef __debug__
        auto endCreationgMapLiteralId2LiteralTime = elapsedTime(startCreationgMapLiteralId2LiteralTime);
        meanTimeIteration = meanTimeIteration / iterationN;
        LOG(LogLevel::INFO_1, "Creation mapLiteralId2Literal time: ", endCreationgMapLiteralId2LiteralTime, " meanTimeIteration: ",meanTimeIteration, " iterationN: ",iterationN);
    #endif

    #ifdef __debug__
        auto startCreationgSoftLiterals = startTimer();
    #endif    

    program->weightFunction.reset(new UMNWeightFunction(program->maxSoftLiteralId + 1));
    program->softLiterals.reset(new Vector<const Literal*>());

    for(const auto&[literalId, weight]: weightDict){
        const Literal* literal = program->mapLiteralId2Literal->literal(literalId);
        program->softLiterals->add(literal);
        program->weightFunction->set(literalId, weight);
    }

    program->preprocessedCost = Program::preprocessCost(*program->softLiterals, *program->weightFunction, program->mapLiteralId2Literal.get());
    program->softLiterals->sort(
        [this](const Literal* a, const Literal* b) noexcept {
            return program->orderSoftLiterals(a, b);
        }
    );
    first = false;

    #ifdef __debug__
        auto endCreationgSoftLiterals = elapsedTime(startCreationgSoftLiterals);
        LOG(LogLevel::INFO_1, "Creation softLiterals time: ", endCreationgSoftLiterals);
    #endif

    return program;
}

bool Program::byDescendingWeight(const Literal* a, const Literal* b) const noexcept {
    const int64_t wa = this->weightFunction->at(a->id), wb = this->weightFunction->at(b->id);
    return wa != wb ? wa > wb : a->id < b->id;
};

void Program::backendAddRule(const Rule* rule) noexcept{
    handleError(clingo_backend_begin(bk));
        handleError(clingo_backend_rule(bk, rule->choice, rule->head.data(), rule->head.size(), rule->body.data(), rule->body.size()));
    handleError(clingo_backend_end(bk));
}


void Program::setPolaritiesLocalSearch(const Assignment* ass, size_t priority, Vector<const Literal *>* literals) noexcept{
    handleError(clingo_control_backend(ctl, &bk));    
    handleError(clingo_backend_begin(bk));
        const size_t p = priority == CONSTANTS::INFPlus32 ? ++priorityPolarity : priority;
        #ifdef __debug__
            std::vector<const Literal*> lits;
        #endif
        const Vector<const Literal*>* literalsToApply = literals == nullptr ? this->softLiterals.get() : literals;
        for(const Literal* l: *literalsToApply){
            const bool& isTrue = ass != nullptr ? ass->isTrue(l) : false;
            const bool& positive = l->positive();
            const clingo_atom_t& id = std::abs(l->id);
            int bias;
            if(positive) 
                bias = isTrue ? 1 : -1;
            else 
                bias = isTrue ? -1 : 1;
            #ifdef __debug__
                if(bias > 0) lits.push_back(this->mapLiteralId2Literal->get(id));
            #endif
            handleError(clingo_backend_heuristic(bk, id,clingo_heuristic_type_sign, bias, p, NULL, 0));
        }
        #ifdef __debug__
            LOG(LogLevel::INFO,"Setting polarity local search with priority: ",priorityPolarity," with: ", lits, " and answer set: ", ass);
        #endif
    handleError(clingo_backend_end(bk));
}

} // namespace fiore
