"""Constants (mirrors fiore/cpp/fiore/utils/constants.h): exit codes and the sentinel literals of the engines."""

SAT_CODE = 10
UNSAT_CODE = 20
OPTIMUM_CODE = 30
OPTIMUM_UNKNOWN_CODE = 29
UNKNOWN_CODE = 28
ERROR_CODE = 1

# the conflict of an implementation: never a solver literal (C++: CONSTANTS::BOTTOM = -1, the negation of the true
# literal 1 of clingo; WASP has no such literal, so Python uses 0)
BOTTOM = 0
# the solver literal of clingo true from the start (C++: CONSTANTS::TOP): watched for the first propagation at level 0
TOP = 1
