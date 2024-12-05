
# starting from directory "propagator_opt"
import os
import subprocess



# name of the file where the weights are
WEIGHTS = "weights"

PROPAGATOR_NAME_ge_eo = "ge_eo"
PROPAGATOR_NAME_ge_amo = "ge_amo"
PROPAGATOR_NAME_le_eo = "le_eo"
PROPAGATOR_MODULE = "propagator_wasp"
PROPAGATORS_NAMES = [PROPAGATOR_NAME_ge_eo, PROPAGATOR_NAME_ge_amo, PROPAGATOR_NAME_le_eo]

ENCODING_WITH_GROUP_GE_EO = "encoding_with_group_ge_eo"
ENCODING_WITH_AGGR_GE_EO = "encoding_with_aggregates_ge_eo"

ENCODING_WITH_GROUP_GE_AMO = "encoding_with_group_ge_amo"
ENCODING_WITH_AGGR_GE_AMO = "encoding_with_aggregates_ge_amo"

ENCODING_WITH_GROUP_LE_EO = "encoding_with_group_le_eo"
ENCODING_WITH_AGGR_LE_EO = "encoding_with_aggregates_le_eo"

ENCODING_WITH_GROUP_LE_AMO = "encoding_with_group_le_amo"
ENCODING_WITH_AGGR_LE_AMO = "encoding_with_aggregates_le_amo"


ROOT = f"{os.path.dirname(os.path.abspath(__file__))}"
PROPAGATOR_DIR_LOCATION_WASP = f"{os.path.dirname(os.path.abspath(__file__))}/prop_wasp"
TESTS_LOCATION = f"{ROOT}/tests"
BENCHMARKS_LOCATION = f"{TESTS_LOCATION}/benchmarks"
TEMP_TESTS_LOCATION = f"{TESTS_LOCATION}/temp"
RESULTS_TESTS_LOCATION = f"{TESTS_LOCATION}/results"
STATISTICS_FOLDER = f"{TESTS_LOCATION}/statistics"
STATISTICS_REASON = f"{STATISTICS_FOLDER}/statistics_reason/"
STATISTICS_RUN = f"{STATISTICS_FOLDER}/statistics_run/"
STATISTICS_REASON_FILE_MINIMAL = f"{STATISTICS_REASON}/output_statistics_reason_minimal"
STATISTICS_REASON_FILE_MINIMUM = f"{STATISTICS_REASON}/output_statistics_reason_minimum"


MAP_ENC_ENCODING_FILES = {
    "ge_amo": (ENCODING_WITH_AGGR_GE_AMO ,ENCODING_WITH_GROUP_GE_AMO),
    "ge_eo":  (ENCODING_WITH_AGGR_GE_EO, ENCODING_WITH_GROUP_GE_EO) ,
    "le_eo":  (ENCODING_WITH_AGGR_LE_EO, ENCODING_WITH_GROUP_LE_EO) }

MAP_PROPAGATOR = {"ge_amo": PROPAGATOR_NAME_ge_amo, "ge_eo": PROPAGATOR_NAME_ge_eo, "le_eo": PROPAGATOR_NAME_le_eo}

FILE_REGEX = r"^\/?[\w.-]+((/[\w.-]*)*[\w+.-])?$"

PREDICATE_GROUP = "__group__"
PREDICATE_LB = "__lb__"
PREDICATE_UB = "__ub__"
PREDICATE_AUX = "__aux__"
