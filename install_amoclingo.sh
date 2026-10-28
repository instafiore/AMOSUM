export PATH="$CONDA_PREFIX/bin:$PATH"
make -C amosum/amoclingo/cpp clean
make -C amosum/amoclingo/cpp release -j
CPP_FOLDER=$(realpath amosum/amoclingo/cpp)
if [[ ":$PATH:" != *"$CPP_FOLDER"* ]]; then
    echo "Installing in $CPP_FOLDER";
    export PATH="$CPP_FOLDER/bin:$PATH";
fi