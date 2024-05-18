
# starting from directory "propagator_opt"
import os
import subprocess

# name of the file where the weights are
WEIGHTS = "weights"

PROPAGATOR_NAME_ge_eo = "propagator_opt_ge_eo"
PROPAGATOR_NAME_ge_amo = "propagator_opt_ge_amo"
PROPAGATOR_NAME_le_eo = "propagator_opt_le_eo"

ENCODING_WITH_GROUP_GE_EO = "encoding_with_group_ge_eo"
ENCODING_WITH_AGGR_GE_EO = "encoding_with_aggregates_ge_eo"

ENCODING_WITH_GROUP_GE_AMO = "encoding_with_group_ge_amo"
ENCODING_WITH_AGGR_GE_AMO = "encoding_with_aggregates_ge_amo"

ENCODING_WITH_GROUP_LE_EO = "encoding_with_group_le_eo"
ENCODING_WITH_AGGR_LE_EO = "encoding_with_aggregates_le_eo"

ENCODING_WITH_GROUP_LE_AMO = "encoding_with_group_le_amo"
ENCODING_WITH_AGGR_LE_AMO = "encoding_with_aggregates_le_amo"

PROPAGATOR_DIR_LOCATION = f"."
TESTS_LOCATION = f"{PROPAGATOR_DIR_LOCATION}/tests"
BENCHMARKS_LOCATION = f"{TESTS_LOCATION}/benchmarks"
TEMP_TESTS_LOCATION = f"{TESTS_LOCATION}/temp"
RESULTS_TESTS_LOCATION = f"{TESTS_LOCATION}/results"

MAP_ENC_ENCODING_FILES = {
    "ge_amo": (ENCODING_WITH_AGGR_GE_AMO ,ENCODING_WITH_GROUP_GE_AMO),
    "ge_eo":  (ENCODING_WITH_AGGR_GE_EO, ENCODING_WITH_GROUP_GE_EO) ,
    "le_eo":  (ENCODING_WITH_AGGR_LE_EO, ENCODING_WITH_GROUP_LE_EO) }

MAP_ENC_PROP = {"ge_amo": PROPAGATOR_NAME_ge_amo, "ge_eo": PROPAGATOR_NAME_ge_eo, "le_eo": PROPAGATOR_NAME_le_eo}
