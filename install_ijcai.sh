make -C amosum/ijcai_version/AMOSUM/prop_clingo/propagator_clingo_c clean
make -C amosum/ijcai_version/AMOSUM/prop_clingo/propagator_clingo_c -j
if [ -n "$1" ]; then
    export WASP_EXE="$1"
fi