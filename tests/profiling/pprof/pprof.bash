OUT=output.prof

# NO EO
# E="/tmp/.encoding-amosum-eo_without_amosum_2026-05-25-17-07-11-637225.asp" 
# EO
E="/tmp/.encoding-amosum-eo_without_amosum_2026-05-25-17-08-42-140245.asp" 

I="/tmp/.0011-knapsack-15-32223-358414-type1_without_amosum_2026-05-25-16-53-53-401234.asp"

# NO EO
# P="ge_eo -id (0,lb(358414,1)) -encoding tests/benchmarks/knapsack/encoding-amosum-eo.asp -instance tests/benchmarks/knapsack/instances/0011-knapsack-15-32223-358414-type1.asp -lazy false -reason nomin -lang cpp -models -log_file log"
# EO
P="ge_eo -id (1,lb(358414,1)) -encoding tests/benchmarks/knapsack/encoding-amosum-eo.asp -instance tests/benchmarks/knapsack/instances/0011-knapsack-15-32223-358414-type1.asp -lazy false -reason nomin -lang cpp -models -log_file log le_eo -id (0,ub(32223,0)) -encoding tests/benchmarks/knapsack/encoding-amosum-eo.asp -instance tests/benchmarks/knapsack/instances/0011-knapsack-15-32223-358414-type1.asp -lazy false -reason nomin -lang cpp -models -log_file log"

CPUPROFILE_FREQUENCY=4000 CPUPROFILE=$OUT timeout 30s \
    /Users/instafiore/Workspace/AMOSUM/amosum/amoclingo/propagator_clingo_c/bin/./amosum_cpp \
    -encoding="$E" \
    -instance="$I" \
    -models=1 \
    -logfile=log \
    -serialize \
    -amosum_propagators="$P"