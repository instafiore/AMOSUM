export PATH="$CONDA_PREFIX/bin:$PATH"
make -C amosum/amoclingo/propagator_clingo_c clean
make -C amosum/amoclingo/propagator_clingo_c release -j
CPP_FOLDER=$(realpath amosum/amoclingo/propagator_clingo_c)
if [[ ":$PATH:" != *"$CPP_FOLDER"* ]]; then
    echo "Installing in $CPP_FOLDER";
    export PATH="$CPP_FOLDER/bin:$PATH";
fi