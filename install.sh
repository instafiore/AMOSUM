#!/usr/bin/env bash
# Installs AMOSUM in the active conda environment, all at once:
#   1. the Python packages: fiore (fiore/python) and amosum (the commands amoclingo, amowasp, amosumparser)
#   2. amoclingo-core, the C++ propagator (amosum/amoclingo/cpp)
#   3. amowasp: the WASP executable, remembered by the environment (WASP_EXE is set at every activation)
# then checks the commands on a small program.
#
# Usage:
#   conda env create -f environment.yml && conda activate amosum
#   ./install.sh [--wasp <path to wasp>] [--no-pip]
#
#   --wasp PATH   the WASP executable (default: $WASP_EXE, or wasp on the PATH); without it amowasp is not set up
#   --no-pip      skip step 1 (the packages are already installed, e.g. to rebuild amoclingo-core only)
set -euo pipefail
cd "$(dirname "$0")"

WASP="${WASP_EXE:-$(command -v wasp || true)}"
PIP=1
while [ $# -gt 0 ]; do
    case "$1" in
        --wasp) WASP="${2:?--wasp needs a path}"; shift 2 ;;
        --wasp=*) WASP="${1#--wasp=}"; shift ;;
        --no-pip) PIP=0; shift ;;
        -h|--help) sed -n '2,13p' "$0" | sed 's/^# \{0,1\}//'; exit 0 ;;
        *) echo "unknown option: $1 (see ./install.sh --help)" >&2; exit 1 ;;
    esac
done

if [ -z "${CONDA_PREFIX:-}" ]; then
    echo "No conda environment is active: run 'conda activate amosum' first (see ./install.sh --help)" >&2
    exit 1
fi
PYTHON="$CONDA_PREFIX/bin/python"
echo "Installing AMOSUM in $CONDA_PREFIX"

# 1. Python packages (editable: the commands use this checkout)
if [ "$PIP" = 1 ]; then
    echo "== 1/3 Python packages (fiore, amosum)"
    "$PYTHON" -m pip install -q -e fiore/python
    "$PYTHON" -m pip install -q -e .
else
    echo "== 1/3 Python packages: skipped (--no-pip)"
fi

# 2. amoclingo-core (found by amoclingo in amosum/amoclingo/cpp/bin)
echo "== 2/3 amoclingo-core"
# only warnings and errors are shown (stderr)
make -s -C amosum/amoclingo/cpp clean > /dev/null
make -s -C amosum/amoclingo/cpp release -j > /dev/null
echo "   built amosum/amoclingo/cpp/bin/amoclingo-core"

# 3. amowasp: WASP_EXE in the environment's activation scripts
echo "== 3/3 amowasp"
ACTIVATE="$CONDA_PREFIX/etc/conda/activate.d/amosum_wasp.sh"
DEACTIVATE="$CONDA_PREFIX/etc/conda/deactivate.d/amosum_wasp.sh"
if [ -n "$WASP" ] && [ -x "$WASP" ]; then
    WASP="$(cd "$(dirname "$WASP")" && pwd)/$(basename "$WASP")"
    mkdir -p "$(dirname "$ACTIVATE")" "$(dirname "$DEACTIVATE")"
    echo "export WASP_EXE=\"$WASP\"" > "$ACTIVATE"
    echo "unset WASP_EXE" > "$DEACTIVATE"
    export WASP_EXE="$WASP"
    echo "   WASP_EXE=$WASP (set at every 'conda activate')"
else
    WASP=""
    echo "   no WASP executable${WASP_EXE:+ at $WASP_EXE}: amowasp is not set up (rerun with --wasp <path to wasp>)"
fi

# check: the three ways to solve, on a program whose answer is known (SAT, exit code 10)
echo "== check"
PROGRAM=tests/benchmarks/edge/ge_sat.asp
check() {
    local name="$1"; shift
    if "$@" "$PROGRAM" > /dev/null 2>&1; then status=0; else status=$?; fi
    if [ "$status" = 10 ]; then echo "   ok   $name"; else echo "   FAIL $name (exit code $status)"; FAILED=1; fi
}
FAILED=0
check "amoclingo (C++)" "$CONDA_PREFIX/bin/amoclingo"
check "amoclingo -lg py (Python on clingo)" "$CONDA_PREFIX/bin/amoclingo" -lg py
if [ -n "$WASP" ]; then check "amowasp (Python on WASP)" "$CONDA_PREFIX/bin/amowasp"; fi
if [ "$FAILED" = 1 ]; then echo "Installation finished with errors" >&2; exit 1; fi

echo "Done.${WASP:+ Reactivate the environment (conda deactivate && conda activate $(basename "$CONDA_PREFIX")) to set WASP_EXE in this shell.}"
