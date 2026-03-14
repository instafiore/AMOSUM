# AMOSUM

dependency:
g++
clingo 5.8.0


install:

pip install .

( In un file )
cd amosum/amoclingo/propagator_clingo_c
make -C amosum/amoclingo/propagator_clingo_c clean
make -C amosum/amoclingo/propagator_clingo_c -j

