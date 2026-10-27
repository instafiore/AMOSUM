#include "utility.h"
#include "constants.h"
#include <assert.h>
#include "logger.h"
#include "stats.h"
#include "argparse.h"


namespace fiore {

std::ostream& operator<<(std::ostream& out, const Cluster& cluster){ return out<<cluster.toString();}

int64_t meanCluster(const Cluster& cluster, UMNWeightFunction* weight){
    int64_t sum = 0;
    const size_t &n = cluster.literals.size();
    for(const Literal* l: cluster.literals) sum += weight->at(l->id);
    return n > 0 ? sum / n : 0;
}


void shuffleWithinClusters(Vector<const Literal*>& targets, const std::vector<size_t>& indexes, std::mt19937& gen) noexcept{
    auto b = targets.begin();
    const size_t clusters = indexes.size() - 1;
    for(size_t j = 0; j < clusters; ++j){
        size_t s = indexes[j];
        size_t e = indexes[j+1];
        if(e-s == 1) continue;
        #ifdef __debug__
            const Literal* lStart = targets.get(s);
            const Literal* lEnd = e < targets.size() ? targets.get(e) : nullptr;
            LOG(LogLevel::INFO, "Shuffling between ",lStart," j: ",s,"  and ",lEnd," j: ",e);
        #endif
        std::shuffle(b+s, b+e, gen);
    }
}


void cleanup() noexcept{
    #ifndef NSTATS
        Statistics* stats = Statistics::getInstance();
    #endif
    ArgParse::cleanup();
    #ifdef __debug__
        #ifndef NSTATS
            LOG(LogLevel::ALWAYS,stats);
        #endif
        LOG(LogLevel::ALWAYS, "Exiting with exit code: ",CONSTANTS::EXIT_CODE);
        Logger::cleanup();
    #endif
    #ifndef NSTATS
        // stats->printOnSTDOUT();
        stats->printOnFile();
    #endif
}

} // namespace fiore
