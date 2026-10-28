OUT=output.prof
bin="/Users/instafiore/Workspace/AMOSUM/amosum/amoclingo/cpp/bin/./amoclingo-core"
pprof --pdf $bin $OUT > ${OUT%.prof}.pdf 2>/dev/null
open ${OUT%.prof}.pdf