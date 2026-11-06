#!/bin/bash
#
# Runs the amosum solver scripts (amoclingo / amowasp, amo and eo encodings,
# #amomaximize) on the benchmark families, collects the results, and checks
# that every completed run printed answer sets of the program it solved
# (encoding with #amosum/#eosum/#amomaximize + instance) with
# check_answer_sets.py.
#
# Settings are the plain variables below; edit them in this file.

cd "$(dirname "$0")" || exit 1

# Directories with the solver scripts. The solver regex is matched against the
# full path of every file directly inside them (not in their subdirectories).
SOLVER_DIRS=(
    /home/mazzotta/Experiments/solvers/amosum/amosum-scripts   # amoclingo, amowasp, ...
    /home/mazzotta/Experiments/solvers                         # clingo, wasp
)
BENCH_DIR=instances
RESULT_HOME=repository-of-results-amosum
NCPU=32
EXEC_DATE=$(date +%Y-%m-%d-%H-%M)


# the checker needs the clingo Python module: the amosum env's Python
PYTHON=/home/mazzotta/miniconda3/envs/amosum/bin/python3
CHECKER=/home/mazzotta/Experiments/solvers/bin_fiorentino/AMOSUM/scripts/check_answer_sets.py
CHECK_TIMEOUT=300s     # time limit for each correctness check
ENABLE_CORRECTNESS=1   # 1 runs the correctness check, 0 skips it
CHECK_UNSAT=0          # 1: an UNSATISFIABLE run is confirmed with clingo (solves the whole instance)
CHECK_OPTIMUM=0        # 1: an OPTIMUM FOUND run is confirmed with clingo (solves the whole optimization: can be slow)

expName="amomaximize"
expDescription="search-conf"
EXEC_ID="$EXEC_DATE${expName:+:$expName}${expDescription:+:$expDescription}"
RESULT_DIR="$RESULT_HOME/$EXEC_ID"

mkdir -p "$RESULT_DIR"

rm -rf results/* stats configuration
mkdir -p results stats configuration

# Pairs "solver-regex-benchmark-regex" to skip. Each entry is split at its
# first "-": the part before is matched against the solver script name, the
# part after against the instance directory name. Currently nothing is skipped.
avoid_conf=()


FAILED_CHECKS=0
TIMEDOUT_CHECKS=0
FAILURES_FILE="$RESULT_DIR/correctness_failures.txt"

# checker_for INSTANCE_PATH SOLVER_NAME: the encoding the solver script runs (empty: not an amosum solver)
checker_for() {
    local instance_path="$1" s="$2"
    echo "$instance_path/encoding-checker.asp"
}

# instance_for INSTANCE_PATH RUN_NAME: the instance file of a run (its .out without the extension)
instance_for() {
    local instance_path="$1" f="$2" candidate
    for candidate in "$instance_path/$f" "$instance_path/$f.asp" "$instance_path/$f.lp"; do
        [[ -f $candidate ]] && { echo "$candidate"; return; }
    done
}

# check_correctness INSTANCE_PATH INSTANCE_DIR_NAME SOLVER_NAME
# For every completed run, every printed answer set must be exactly an answer
# set of encoding + instance (check_answer_sets.py, mode exact: amosum prints
# all the atoms), with the printed #amomaximize values; UNSAT and the optimum
# are confirmed with clingo if CHECK_UNSAT / CHECK_OPTIMUM.
#
# Only runs with status complete are considered, by their exit code:
#   10, 20, 30   SAT, UNSAT, OPTIMUM FOUND: checked
#   1            an error: the whole experiment stops
#   others       not checked
#
# The messages go to $RESULT_DIR/INSTANCE_DIR_NAME.SOLVER_NAME/correctness/correctness.log.
check_correctness() {
    local instance_path="$1" instance_dir_name="$2" s="$3"
    local res_dir="$RESULT_DIR/$instance_dir_name.$s"
    local log="$res_dir/correctness/correctness.log"
    local filePyrl file f rc run_rc encoding instance
    local check_args=(--strict)
    [[ $CHECK_UNSAT == 1 ]] && check_args+=(--check-unsat)
    [[ $CHECK_OPTIMUM == 1 ]] && check_args+=(--check-optimum)

    # plain clingo / wasp: not amosum's output format
    if [[ $s == clingo* || $s == wasp* ]]; then
        return
    fi

    encoding=$(checker_for "$instance_path" "$s")
    [[ -z $encoding ]] && return           # not an amosum solver script

    mkdir -p "$res_dir/correctness"
    exec 3>>"$log"

    if [[ ! -f "$encoding" ]]; then
        echo "Encoding $encoding not found, skipping correctness test for $s on $instance_dir_name" >&3
        echo "$s $instance_dir_name (missing encoding $encoding)" >> "$FAILURES_FILE"
        FAILED_CHECKS=$((FAILED_CHECKS + 1))
        exec 3>&-
        return
    fi

    echo "Correctness test for $s on $instance_dir_name during $EXEC_ID (encoding: $(basename "$encoding"))" >&3
    for filePyrl in "$res_dir"/*.pyrl; do
        [[ -e "$filePyrl" ]] || continue
        [[ $filePyrl == *encoding* ]] && continue
    


        file="${filePyrl%.pyrl}.out"
        f=$(basename "${filePyrl%.pyrl}")

        # exit code of the run, from a line like "[pyrunlim] result:		10"
        run_rc=$(grep -E "\[pyrunlim\] result:" "$filePyrl" | awk '{print $NF}' | tail -n 1)
        case "$run_rc" in
            10|20|30|29)
                ;;
            1)
                echo "Run $file completed with exit code 1 (error): stopping the experiment" >&3
                echo "$s $instance_dir_name $f (run exit code 1)" >> "$FAILURES_FILE"
                exec 3>&-
                echo "Run $file completed with exit code 1: experiment stopped, see $log" >&2
                exit 1
                ;;
            *)
                echo "Not checked: $file (exit code ${run_rc:-missing})" >&3
                continue
                ;;
        esac

        instance=$(instance_for "$instance_path" "$f")
        if [[ -z $instance ]]; then
            echo "Instance of $file not found in $instance_path" >&3
            echo "$s $instance_dir_name $f (instance not found)" >> "$FAILURES_FILE"
            FAILED_CHECKS=$((FAILED_CHECKS + 1))
            continue
        fi

        timeout "$CHECK_TIMEOUT" "$PYTHON" "$CHECKER" "${check_args[@]}" -p "$encoding" --mode=facts "$file" >&3 2>&1
        rc=$?
        if [[ $rc -eq 124 ]]; then
            echo "Check timed out ($CHECK_TIMEOUT) for $file" >&3
            echo "$s $instance_dir_name $f (check timed out)" >> "$FAILURES_FILE"
            TIMEDOUT_CHECKS=$((TIMEDOUT_CHECKS + 1))
        elif [[ $rc -ne 0 ]]; then
            echo "Error detected for instance $file (exit code $rc)" >&3
            echo "$s $instance_dir_name $f (exit code $rc)" >> "$FAILURES_FILE"
            FAILED_CHECKS=$((FAILED_CHECKS + 1))
        else
            echo "Check passed for $instance_path $s $f" >&3
        fi
    done

    exec 3>&-
}

run_benchmark() {
    local filter_regex="$1"
    local benchmark="$2"
    local solvers="$3"
    local dirs

    if [[ "$benchmark" == "Normal" ]]; then
        dirs=$(ls -d $BENCH_DIR/* | grep -v -E "Lite$|Subset$" | grep -E "$filter_regex")
    else
        dirs=$(ls -d $BENCH_DIR/* | grep -P "$benchmark$" | grep -E "$filter_regex")
    fi

    for i in $dirs; do
        INSTANCE_DIR_NAME=$(basename "$i")
        echo "solvers regex: $solvers"
        for sPath in $(find "${SOLVER_DIRS[@]}" -mindepth 1 -maxdepth 1 -type f | sort | grep -P "$solvers"); do
            chmod u=rwx "$sPath"
            s=$(basename "$sPath")

            for avoid in "${avoid_conf[@]}"; do
                regex_solved="${avoid%%-*}"
                benchmark_solved="${avoid#*-}"
                if [[ "$s" =~ $regex_solved ]] && [[ "$INSTANCE_DIR_NAME" =~ $benchmark_solved ]]; then
                    echo "Avoiding $s on $INSTANCE_DIR_NAME during $EXEC_ID"
                    continue 2
                fi
            done

            echo "Executing $s on $INSTANCE_DIR_NAME during $EXEC_ID"
            mkdir -p "results/$INSTANCE_DIR_NAME"
			rm -rf stats configuration && mkdir -p stats configuration
            ./parallelRunner.bash "$NCPU" "$INSTANCE_DIR_NAME" "$sPath"
            echo "Extracting $s on $INSTANCE_DIR_NAME during $EXEC_ID"
            ./extractorFiore.bash "results/$INSTANCE_DIR_NAME" "res.$INSTANCE_DIR_NAME.$s.txt" "$s" > "res.extractor.$INSTANCE_DIR_NAME.$s.txt"
            mv "res.$INSTANCE_DIR_NAME.$s.txt" "res.extractor.$INSTANCE_DIR_NAME.$s.txt" "$RESULT_DIR"

            mv "results/$INSTANCE_DIR_NAME" "$RESULT_DIR/$INSTANCE_DIR_NAME.$s"
            mv stats "$RESULT_DIR/$INSTANCE_DIR_NAME.$s/stats"
            mv configuration "$RESULT_DIR/$INSTANCE_DIR_NAME.$s/configuration"
            

            if [[ "$ENABLE_CORRECTNESS" == "1" ]]; then
                check_correctness "$i" "$INSTANCE_DIR_NAME" "$s"
            fi

            rm -f "$RESULT_DIR/$INSTANCE_DIR_NAME.$s"/*.out
            # rm "$RESULT_DIR/$INSTANCE_DIR_NAME.$s"/*.pyrl
        done
    done
}

# decision problems: amo and eo encodings, on clingo and wasp (plus the plain clingo baseline)
benchs="CA$|KnapsackAmomaximize|TSP|WeightedGraphColouringAmomaximize|WOD"
# solvers='.*/(amo|eo)(clingo)-reas=(nomi|minf)-lazy=true'
solvers='.*/(amoclingo-reas=nomi-lazy=false|amoclingo-reas=minf-lazy=true|clingo-plain-maximize-amo)'
# run_benchmark "$benchs" "Lite" "$solvers"
run_benchmark "$benchs" "Normal" "$solvers"


if [[ $FAILED_CHECKS -eq 0 && $TIMEDOUT_CHECKS -eq 0 ]]; then
    echo "Check passed for all!"
else
    echo "$FAILED_CHECKS correctness check(s) failed, $TIMEDOUT_CHECKS timed out, see $FAILURES_FILE"
    [[ $FAILED_CHECKS -eq 0 ]] || exit 1
fi
