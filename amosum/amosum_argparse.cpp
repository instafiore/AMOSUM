#include "amosum_argparse.h"

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
            .withLongDesc("Search algorithm for #amomaximize: lsu (linear search, each model of value v sets the "
                          "bound to v+1 until UNSAT). Ignored without #amomaximize")
            .withDefaultValue(AMOSUM_ARGS::algorithmDefault)
            .withValidValues({AMOSUM_ARGS::algorithmLsu})
            .build()
    };
}
