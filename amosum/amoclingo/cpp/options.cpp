#include "amosum/amoclingo/cpp/options.h"

std::vector<fiore::Argument> amosumArguments() noexcept{
    using fiore::Argument;
    return {
        Argument::Builder(AMOSUM_ARGS::reasonKey).isWithValue()
            .withDesc("Reasons computation")
            .withLongDesc("Reasons computation: nomin (from the second maximum of the derived literal's group), "
                          "ijcai (all the false literals of that group), min and minfly (nomin minimized greedily), "
                          "cmin (not implemented: as nomin)")
            .withDefaultValue(AMOSUM_ARGS::reasonDefault)
            .withValidValues({AMOSUM_ARGS::reasonNomin, AMOSUM_ARGS::reasonIjcai, AMOSUM_ARGS::reasonMin, AMOSUM_ARGS::reasonMinfly, AMOSUM_ARGS::reasonCmin})
            .build(),
        Argument::Builder(AMOSUM_ARGS::lazyKey).isWithValue()
            .withDesc("When to propagate: false, true, hybrid, or a float in [0,1]")
            .withLongDesc("When to propagate: false and hybrid when a single group can still bring the possible sum "
                          "across the bound, true and a float p when lb/mps (>=) or mps/ub (<=) reaches 1 or p; a "
                          "violated bound is a conflict in every mode")
            .withDefaultValue(AMOSUM_ARGS::lazyDefault)
            .withRegex(LazyValue())
            .build(),
        Argument::Builder(AMOSUM_ARGS::algorithmKey).isWithValue()
            .withDesc("Search algorithm for #amomaximize")
            .withLongDesc("Search algorithm for #amomaximize: sat-unsat (upwards from the minimum possible value: "
                          "each model of value v sets the bound to v+p(step), back to v+1 after UNSAT), unsat-sat "
                          "(downwards from the maximum possible value: each UNSAT bound u sets it to u-p(step), back to "
                          "u-1 after a model); p is --progression (1: LSU and LUS); binary (binary search between the "
                          "best model's value and the smallest bound known UNSAT; --progression is not used). Ignored "
                          "without #amomaximize")
            .withDefaultValue(AMOSUM_ARGS::algorithmDefault)
            .withValidValues({AMOSUM_ARGS::algorithmSatUnsat, AMOSUM_ARGS::algorithmUnsatSat, AMOSUM_ARGS::algorithmBinary})
            .build(),
        Argument::Builder(AMOSUM_ARGS::progressionKey).isWithValue()
            .withDesc("Step of the bound of sat-unsat / unsat-sat at step n: 1 (linear), 2 (+2), 2^n, n^2, ...")
            .withLongDesc("Step of the bound of --opt-strategy at step n = 0, 1, 2, ...: an expression in n with "
                          "integers, + - * ^ and parentheses, e.g. 1 (linear), 2 (+2), 2^n, n^2; values below 1 count "
                          "as 1. Step n restarts from 0 when the search goes back (UNSAT for sat-unsat, a model for unsat-sat)")
            .withDefaultValue(AMOSUM_ARGS::progressionDefault)
            .withRegex(ProgressionValue())
            .build()
    };
}
