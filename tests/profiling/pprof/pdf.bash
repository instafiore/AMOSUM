OUT=output.prof
bin="/Users/instafiore/Workspace/AMOSUM/amosum/amoclingo/propagator_clingo_c/bin/./amosum_cpp"
pprof --pdf $bin $OUT > ${OUT%.prof}.pdf 2>/dev/null
open ${OUT%.prof}.pdf