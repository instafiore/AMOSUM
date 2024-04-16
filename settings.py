
# starting from directory "propagator_opt"
import os

PROPAGATOR_NAME_ge_eo = "propagator_opt_ge_eo"
PROPAGATOR_NAME_ge_amo = "propagator_opt_ge_amo"
PROPAGATOR_NAME_le_eo = "propagator_opt_le_eo"

TYPE_GE_EO = "ge_eo"
TYPE_GE_AMO = "ge_amo"
TYPE_LE_EO = "le_eo"

ENCODING_WITH_GROUP_GE_EO = "encoding_with_group_ge_eo"
ENCODING_WITH_GROUP_GE_AMO = "encoding_with_group_ge_amo"
ENCODING_WITH_GROUP_LE_EO = "encoding_with_group_le_eo"
ENCODING_WITH_AGGR_GE_EO = "encoding_with_aggregates_ge_eo"

PROPAGATOR_DIR_LOCATION = f"."
TESTS_LOCATION = f"{PROPAGATOR_DIR_LOCATION}/tests"
BENCHMARKS_LOCATION = f"{TESTS_LOCATION}/benchmarks"
TEMP_TESTS_LOCATION = f"{TESTS_LOCATION}/temp"
RESULTS_TESTS_LOCATION = f"{TESTS_LOCATION}/results"
WEIGHTS_GC_TESTS_LOCATION = f"{BENCHMARKS_LOCATION}/graph_colouring/weights.asp"