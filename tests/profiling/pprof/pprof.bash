OUT=output.prof

# NO EO
# E="/tmp/.encoding-amosum-eo_without_amosum_2026-05-25-17-07-11-637225.asp" 
# EO
E="/tmp/.encoding-amosum-amo_without_amosum_2026-06-03-12-51-26-392122.asp" 

I="/tmp/.0011-knapsack-15-32223-358414-type1_without_amosum_2026-06-03-12-51-26-431086.asp"

# NO EO
# P="ge_eo -id (0,lb(358414,1)) -encoding tests/benchmarks/knapsack/encoding-amosum-eo.asp -instance tests/benchmarks/knapsack/instances/0011-knapsack-15-32223-358414-type1.asp -lazy false -reason nomin -lang cpp -models -log_file log"
# EO
P="ge_amo -id (1,lb(358414,1)) -encoding /Users/instafiore/Workspace/AMOSUM/tests/benchmarks/knapsack/encoding-amosum-amo.asp -instance /Users/instafiore/Workspace/AMOSUM/tests/benchmarks/knapsack/instances/0011-knapsack-15-32223-358414-type1.asp -lazy false -reason nomin -lang cpp -models -log_file log -static_mpc false le_amo -id (0,ub(32223,0)) -encoding /Users/instafiore/Workspace/AMOSUM/tests/benchmarks/knapsack/encoding-amosum-amo.asp -instance /Users/instafiore/Workspace/AMOSUM/tests/benchmarks/knapsack/instances/0011-knapsack-15-32223-358414-type1.asp -lazy false -reason nomin -lang cpp -models -log_file log -static_mpc false"

CPUPROFILE_FREQUENCY=4000 CPUPROFILE=$OUT timeout 30s \
    /Users/instafiore/Workspace/AMOSUM/amosum/amoclingo/propagator_clingo_c/bin/./amosum_cpp \
    -encoding="$E" \
    -instance="$I" \
    -models=1 \
    -logfile=log \
    -serialize \
    -amosum_propagators="$P"