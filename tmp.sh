set -e
export PATH="$CONDA_PREFIX/bin:$PATH"
# ─── Configuration ────────────────────────────────────────────────────────────
WASP_REPO="https://github.com/alviano/wasp"
WASP_DIR="wasp"
BUILD_DIR="build/release"
SCRIPT="python38"
MAKEFILE="$WASP_DIR/Makefile"
ROOT=$(pwd)

# # ─── Clone ────────────────────────────────────────────────────────────────────
# echo "Cloning wasp from $WASP_REPO..."
git clone "$WASP_REPO" "$WASP_DIR"
sed -i.bak 's/cxxflags.release = -Wall -Wextra -std=c++11 -DNDEBUG -O3/cxxflags.release = -Wall -Wextra -std=c++11 -DNDEBUG -O3 -I$(CONDA_PREFIX)\/include/' $MAKEFILE
sed -i.bak 's/linkflags.release = -lm -ldl/linkflags.release = -lm -ldl -L$(CONDA_PREFIX)\/lib -Wl,-rpath,$(CONDA_PREFIX)\/lib/' $MAKEFILE
sed -i.bak 's|scriptsc\.python38 = $(shell python3-config --cflags|scriptsc.python38 = -I$(CONDA_PREFIX)/include $(shell $(CONDA_PREFIX)/bin/python3-config --cflags|' $MAKEFILE
sed -i.bak 's|scriptsld\.python38 = $(shell python3-config --ldflags --embed|scriptsld.python38 = $(shell $(CONDA_PREFIX)/bin/python3-config --ldflags --embed|' $MAKEFILE
rm -f $MAKEFILE.bak

# ─── Build ────────────────────────────────────────────────────────────────────
echo "Building wasp with SCRIPT=$SCRIPT"
make -C $WASP_DIR clean 
make -C $WASP_DIR SCRIPT="$SCRIPT" -j

export PATH="$PATH:$ROOT/$WASP_DIR/$BUILD_DIR"
echo "Done. wasp installed at $ROOT/$WASP_DIR/$BUILD_DIR."
