#pragma once
#include <string>
#include <vector>
#include "fiore/utils/utility.h"
#include "fiore/utils/argparse.h"

// command line of the amosum solver (amosum_lsu): amosum's own options, parsed by fiore::ArgParse together with fiore's
// ones (--log, --log-file-name, --statsfile, --seed, ...); the ASP files are the positional arguments
namespace AMOSUM_ARGS {

    // --reason: reasons of the >= derivations (see AmoclingoPropagator)
    const std::string reasonKey = "reason";
    const std::string reasonNomin = "nomin";
    const std::string reasonIjcai = "ijcai";
    const std::string reasonMin = "min";
    const std::string reasonMinfly = "minfly";
    const std::string reasonCmin = "cmin";
    const std::string reasonDefault = reasonNomin;

    // --lazy: when to propagate (see AmoclingoPropagator): false, true, hybrid, or a float in [0,1]
    const std::string lazyKey = "lazy";
    const std::string lazyDefault = "false";


    // --algorithm: search algorithm for #amomaximize (without #amomaximize: always a single solve, as lsu does)
    const std::string algorithmKey = "opt-strategy";
    const std::string algorithmLsu = "lsu";
    const std::string algorithmDefault = algorithmLsu;
}

// false, true, hybrid, or a float in [0,1]
struct LazyValue: public fiore::PrintableRegex{
    LazyValue(): fiore::PrintableRegex("(false|true|hybrid|0(\\.\\d+)?|1(\\.0+)?)"){}
};

// amosum's options, to pass to fiore::ArgParse::init
std::vector<fiore::Argument> amosumArguments() noexcept;


