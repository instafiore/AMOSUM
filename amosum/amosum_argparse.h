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


    // --opt-strategy: search algorithm for #amomaximize (without #amomaximize: always a single solve call)
    const std::string algorithmKey = "opt-strategy";
    const std::string algorithmSatUnsat = "sat-unsat";     // upwards (LSU with --progression 1)
    const std::string algorithmUnsatSat = "unsat-sat";     // downwards (LUS with --progression 1)
    const std::string algorithmDefault = algorithmSatUnsat;

    // --progression: how far the bound of sat-unsat / unsat-sat moves at step n (an expression in n, see Progression)
    const std::string progressionKey = "progression";
    const std::string progressionDefault = "1";
}

// integers, n, + - * ^, parentheses and spaces (the expression itself is checked by Progression)
struct ProgressionValue: public fiore::PrintableRegex{
    ProgressionValue(): fiore::PrintableRegex("[0-9n+\\-*^() ]+"){}
};

// false, true, hybrid, or a float in [0,1]
struct LazyValue: public fiore::PrintableRegex{
    LazyValue(): fiore::PrintableRegex("(false|true|hybrid|0(\\.\\d+)?|1(\\.0+)?)"){}
};

// amosum's options, to pass to fiore::ArgParse::init
std::vector<fiore::Argument> amosumArguments() noexcept;


