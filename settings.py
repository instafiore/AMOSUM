
# starting from directory "WASP_HOME"
import os

PROPAGATOR_NAME_e = "propagator_opt_e"
PROPAGATOR_NAME_le = "propagator_opt_le"

ENCODING_WITH_GROUP_E = "encoding_with_group_e"
ENCODING_WITH_GROUP_LE = "encoding_with_group_le"
ENCODING_WITH_AGGR = "encoding_with_aggregates"

WASP_HOME = os.environ.get('WASP_HOME')
PROPAGATOR_DIR_LOCATION = f"."
TESTS_LOCATION = f"{PROPAGATOR_DIR_LOCATION}/tests"
BENCHMARKS_LOCATION = f"{TESTS_LOCATION}/benchmarks"
TEMP_TESTS_LOCATION = f"{TESTS_LOCATION}/temp"
RESULTS_TESTS_LOCATION = f"{TESTS_LOCATION}/results"
WEIGHTS_TESTS_LOCATION = f"{BENCHMARKS_LOCATION}/graph_colouring/weights.asp"