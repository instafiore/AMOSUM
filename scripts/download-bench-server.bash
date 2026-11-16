LOCAL_BENCHDIR=/Users/instafiore/Workspace/benchmarks/
SERVER_BENCHDIR=clown:/home/mazzotta/Experiments/instances/

includes=()
for i in CombinatorialAuctionsOpt KnapsackOpt TspOpt GraphColouringOpt WOD;do
    includes+=("--include=$i/ --include=$i/*")
done

rsync -avz -P ${includes[@]} --exclude="*" $SERVER_BENCHDIR $LOCAL_BENCHDIR