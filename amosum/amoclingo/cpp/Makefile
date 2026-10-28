# CC = g++
CC = $(CXX)
CFLAGS = -Wall -Wextra -std=c++20 -I/opt/homebrew/include -I$(CONDA_PREFIX)/include $(EXTRA_CFLAGS) -Wno-unused-parameter -Wno-comment -Wno-unused-variable -Wno-unused-label -Wno-unused-command-line-argument -Wno-unused-but-set-variable -Wno-missing-field-initializers
LDFLAGS = -L/opt/homebrew/lib -L$(CONDA_PREFIX)/lib -Wl,-rpath,/opt/homebrew/lib -Wl,-rpath,$(CONDA_PREFIX)/lib -lclingo $(EXTRA_LDFLAGS)
BUILD_DIR = build
BIN_DIR = bin
REPO := $(abspath ../../..)
TARGET = amoclingo-core
C=cpp

# the sources of amoclingo-core: amoclingo/cpp (main, runner, options, clingo propagator and solver) and core/cpp
# (program, propagation, searches); objects keep their folders under build/ (both have a propagator.cpp)
SRCS = $(wildcard $(REPO)/amosum/amoclingo/cpp/*.$(C)) $(wildcard $(REPO)/amosum/core/cpp/*.$(C)) $(wildcard $(REPO)/amosum/core/cpp/search/*.$(C))
OBJS = $(patsubst $(REPO)/%.$(C),$(BUILD_DIR)/%.o,$(SRCS))
# fiore (fiore/cpp): utilities, Clingo helpers and the model (Program, ...), compiled from source into build/fiore/
include ../../../fiore/cpp/fiore.mk
# includes: "fiore/..." (FIORE_CPPFLAGS) and "amosum/..." (the repository root)
CFLAGS += $(FIORE_CPPFLAGS) -I$(REPO) -MMD -MP
FIORE_OBJS = $(patsubst $(FIORE_DIR)/%.$(C),$(BUILD_DIR)/fiore/%.o,$(FIORE_SRCS))

all: $(TARGET)

print-CC:
	@echo "Using $(CC)"

BUILD_MODE ?= release
ifeq ($(BUILD_MODE), test)
#     EXTRA_CFLAGS  = -fsanitize=address -g -O0
#     EXTRA_LDFLAGS = -fsanitize=address
	EXTRA_CFLAGS  = -g -O0
    EXTRA_LDFLAGS = 
else ifeq ($(BUILD_MODE), pprof)
    EXTRA_CFLAGS  = -O2 -g -DNDEBUG
    EXTRA_LDFLAGS = -lprofiler -ltcmalloc
else
    EXTRA_CFLAGS  = -O3 -DNDEBUG
    EXTRA_LDFLAGS = 
endif
	
test:
	@$(MAKE) BUILD_MODE=test

release:
	@$(MAKE) BUILD_MODE=release

pprof:
	@$(MAKE) BUILD_MODE=pprof


$(TARGET): $(OBJS) $(FIORE_OBJS)
	@echo "Linking $@"
	@mkdir -p $(BIN_DIR)
	$(CC) $^ $(LDFLAGS) -o $(BIN_DIR)/$@


$(BUILD_DIR)/fiore/%.o: $(FIORE_DIR)/%.$(C)
	@mkdir -p $(dir $@)
	@echo "Compiling $< -> $@"
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/%.o: $(REPO)/%.$(C)
	@mkdir -p $(dir $@)
	@echo "Compiling $< -> $@"
	$(CC) $(CFLAGS) -c $< -o $@

# header dependencies (-MMD): a changed header recompiles the files that include it
-include $(OBJS:.o=.d) $(FIORE_OBJS:.o=.d)

clean:
	@echo "Cleaning up..."
	@rm -rf $(BUILD_DIR) $(BIN_DIR)
