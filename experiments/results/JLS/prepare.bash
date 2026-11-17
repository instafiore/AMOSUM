#!/bin/bash
#
# Downloads the result files (res.<bench>.<solver>.txt) of one experiment from
# the server and joins them into results_tsv/<experiment>.tsv, one line per
# run:
#   experiment  bench  instance  solver  <fields of the res line, without the
#   empty second field and the "unk" fields>
#
# Usage: ./prepare.bash [EXPERIMENT] [RSYNC_OPTION...]
#   EXPERIMENT is the folder name in repository-of-results-amosum on the
#   server (default: the latest one, by the date at the start of the name);
#   extra options go to rsync, e.g. -n for a dry run (then no TSV is written).
#   An existing TSV is saved as <experiment>.tsv.bak before it is rewritten.

cd "$(dirname "$0")" || exit 1

SERVER=clown
SERVER_RESULT_HOME=/home/mazzotta/Experiments/repository-of-results-amosum
if [[ $# -gt 0 && $1 != -* ]]; then
    RESULT_DIR="$1"
    shift
else
    # the latest experiment: folder names start with the date (YYYY-MM-DD-HH-MM),
    # so the last one in alphabetical order is the newest
    RESULT_DIR=$(ssh "$SERVER" "cd '$SERVER_RESULT_HOME' && ls -1d 202*/" | sort | tail -n 1)
    RESULT_DIR=${RESULT_DIR%/}
    if [[ -z $RESULT_DIR ]]; then
        echo "No experiment found in $SERVER:$SERVER_RESULT_HOME" >&2
        exit 1
    fi
    echo "Latest experiment: $RESULT_DIR"
fi

LOCAL_RESULT_RES=results_res
RESULT_TSV=results_tsv

OUT="$LOCAL_RESULT_RES/$RESULT_DIR"
OUTPUT="$RESULT_TSV/$RESULT_DIR.tsv"
mkdir -p "$OUT" "$RESULT_TSV"

# Only the res.* files directly in the experiment folder; the extractor logs
# (res.extractor.*) are excluded first, since the first matching rule wins.
FILTER=(--exclude='res.extractor.*' --include='/res.*' --exclude='*')
rsync -av "${FILTER[@]}" "$@" "$SERVER:$SERVER_RESULT_HOME/$RESULT_DIR/" "$OUT/" || exit 1

# dry run: nothing was downloaded, so the TSV is not rewritten
for opt in "$@"; do
    if [[ $opt == -n || $opt == --dry-run ]]; then
        echo "Dry run: $OUTPUT not written"
        exit 0
    fi
done

shopt -s nullglob
files=("$OUT"/res.*)
if [[ ${#files[@]} -eq 0 ]]; then
    echo "No res.* files in $OUT" >&2
    exit 1
fi

if [[ -f $OUTPUT ]]; then
    cp "$OUTPUT" "$OUTPUT.bak"
    echo "Old TSV saved as $OUTPUT.bak"
fi

echo "Writing into -> $OUTPUT"
for f in "${files[@]}"; do
    # res.<bench>.<solver>.<ext>.txt: the bench stops at its first dot, the
    # solver is the rest without .txt and the script extension
    # (clingo-amo.bash.txt -> clingo-amo, amoclingo-...-p=0.7.sh.txt -> amoclingo-...-p=0.7)
    name=$(basename "$f")
    name=${name#res.}
    bench=${name%%.*}
    solver=${name#"$bench".}
    solver=${solver%.txt}
    solver=${solver%.*}

    # Each res line is tab separated: the .out path of the run, an empty field,
    # then the status, times, memory and the extractor fields. Skipped: the
    # encodings/checkers and the CP model files, which are not instances.
    # Dropped: the second field (always empty) and the "unk" fields.
    awk -F'\t' -v OFS='\t' -v expid="$RESULT_DIR" -v bench="$bench" -v solver="$solver" '
        {
            inst = $1
            sub(/^[ \t]+/, "", inst)
            sub(/.*\//, "", inst)
            sub(/\.out$/, "", inst)
            if (inst ~ /encoding|checker/ || inst ~ /^(model\.(py|mzn)|cpsat_cli\.py)$/)
                next
            line = expid OFS bench OFS inst OFS solver OFS
            for (i = 1; i <= NF; i++)
                if ($i != "unk")
                    line = line $i OFS
            print line
        }' "$f"
done > "$OUTPUT"

echo "$(wc -l < "$OUTPUT" | tr -d ' ') runs from ${#files[@]} files"

# every line must have the same number of columns (lib.read_tsv expects 15)
ncols=$(awk -F'\t' '{print NF}' "$OUTPUT" | sort -u)
if [[ $(wc -l <<< "$ncols") -gt 1 ]]; then
    echo "WARNING: lines with a different number of columns:" >&2
    awk -F'\t' '{print NF}' "$OUTPUT" | sort | uniq -c >&2
fi