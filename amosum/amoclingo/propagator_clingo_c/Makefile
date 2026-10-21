# CC = g++
CC = $(CXX)
CFLAGS = -Wall -Wextra -std=c++20 -I/opt/homebrew/include -I$(CONDA_PREFIX)/include $(EXTRA_CFLAGS) -Wno-unused-parameter -Wno-comment -Wno-unused-variable -Wno-unused-label -Wno-unused-command-line-argument -Wno-unused-but-set-variable -Wno-missing-field-initializers
LDFLAGS = -L/opt/homebrew/lib -L$(CONDA_PREFIX)/lib -Wl,-rpath,/opt/homebrew/lib -Wl,-rpath,$(CONDA_PREFIX)/lib -lclingo $(EXTRA_LDFLAGS)
BUILD_DIR = build
BIN_DIR = bin
INCLUDES =  
SRC_DIR = ../..
TARGET = amosum_cpp
C=cpp
amowasp = $(SRC_DIR)/amowasp/propagator_wasp_c

SRCS = $(shell find $(SRC_DIR) -name '*.$(C)')
OBJS = $(addprefix $(BUILD_DIR)/, $(notdir $(SRCS:.$(C)=.o)))
# two programs from the same sources: amosum_cpp (runner_clingo.cpp, the old propagators) and amosum_lsu
# (runner_lsu.cpp, AmosumClingoSolver on fiore): each one without the other's main
LSU_TARGET = amosum_lsu
OBJS_CPP = $(filter-out $(BUILD_DIR)/runner_lsu.o, $(OBJS))
OBJS_LSU = $(filter-out $(BUILD_DIR)/runner_clingo.o, $(OBJS))

# fiore (repository root): utilities, Clingo helpers and the model (Program, ...), compiled from source into build/fiore/
include ../../../fiore/fiore.mk
CFLAGS += $(FIORE_CPPFLAGS)
FIORE_OBJS = $(patsubst $(FIORE_DIR)/%.$(C),$(BUILD_DIR)/fiore/%.o,$(FIORE_SRCS))

all: $(TARGET) $(LSU_TARGET)

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


$(TARGET): $(OBJS_CPP) $(FIORE_OBJS)
	@echo "Linking $@"
	@mkdir -p $(BIN_DIR)
	$(CC) $^ $(LDFLAGS) -o $(BIN_DIR)/$@

$(LSU_TARGET): $(OBJS_LSU) $(FIORE_OBJS)
	@echo "Linking $@"
	@mkdir -p $(BIN_DIR)
	$(CC) $^ $(LDFLAGS) -o $(BIN_DIR)/$@

$(BUILD_DIR)/fiore/%.o: $(FIORE_DIR)/%.$(C)
	@mkdir -p $(dir $@)
	@echo "Compiling $< -> $@"
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.$(C)
	@mkdir -p $(BUILD_DIR) # Create build directory if not exists
	@echo "Compiling $< -> $@"
	$(CC) $(CFLAGS) $(LDFLAGS) -c $< -o $@

$(BUILD_DIR)/%.o: $(amowasp)/%.$(C)
	@mkdir -p $(BUILD_DIR) # Create build directory if not exists
	@echo "Compiling $< -> $@"
	$(CC) $(CFLAGS) $(LDFLAGS) -c $< -o $@

$(BUILD_DIR)/%.o: ./%.$(C)
	@mkdir -p $(BUILD_DIR) # Create build directory if not exists
	@echo "Compiling $< -> $@"
	$(CC) $(CFLAGS) $(LDFLAGS) -c $< -o $@

clean:
	@echo "Cleaning up..."
	@rm -rf $(BUILD_DIR) $(BIN_DIR)
