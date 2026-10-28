#include <clingo.h>
#include <stdlib.h>
#include <stdio.h>
#include <string>
#include <assert.h>
#include "amosum/core/cpp/utility.h"
#include <sstream>
#include <iostream>
#include <vector>
#include <limits>
#include <chrono>
#include <csignal>
#include <sys/signal.h>
#include "fiore/utils/constants.h"
#include "fiore/utils/logger.h"
#include "fiore/utils/utility.h"
#include "amosum/amoclingo/cpp/propagator.h"
#include "fiore/model/propagatorPBConstraint.h"
#include "fiore/model/assigment.h"
#include "fiore/utils/utilityCommon.h"
#include "fiore/utils/argparse.h"
#include "amosum/amoclingo/cpp/options.h"
#include "fiore/utils/stats.h"
#include "fiore/clingo/utilityClingo.h"




using namespace Args;
using fiore::operator<<;

struct Runner{
    fiore::clingo::ClingoResult run();
};
