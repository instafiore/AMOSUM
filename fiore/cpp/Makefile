# Builds and runs fiore's tests. Applications do not use this file: they include fiore.mk.
#
#   make test            release and debug
#   make test-release    -O3 -DNDEBUG
#   make test-debug      -g -O0 (asserts and LOG enabled)
#   make STATS=0 ...     compile Statistics out (-DNSTATS)
#   make clean

include fiore.mk

CXX ?= c++
CLINGO_INC = -I/opt/homebrew/include
CLINGO_LIB = -L/opt/homebrew/lib -Wl,-rpath,/opt/homebrew/lib
ifneq ($(CONDA_PREFIX),)
    CLINGO_INC += -I$(CONDA_PREFIX)/include
    CLINGO_LIB += -L$(CONDA_PREFIX)/lib -Wl,-rpath,$(CONDA_PREFIX)/lib
endif

BUILD_MODE ?= release
ifeq ($(BUILD_MODE), debug)
    MODE_FLAGS = -g -O0
else
    MODE_FLAGS = -O3 -DNDEBUG
endif
STATS ?= 1
ifeq ($(STATS), 0)
    MODE_FLAGS += -DNSTATS
endif

CXXFLAGS = -std=c++20 -Wall -Wextra -Wno-unused-parameter -Wno-missing-field-initializers -Wno-macro-redefined $(MODE_FLAGS) $(FIORE_CPPFLAGS)
OUT = build/$(BUILD_MODE)

# utilities: compiled and linked without any Clingo include path or library
UTILS_OBJS  = $(FIORE_UTILS_SRCS:$(FIORE_DIR)/%.cpp=$(OUT)/obj/%.o)
CLINGO_OBJS = $(FIORE_CLINGO_SRCS:$(FIORE_DIR)/%.cpp=$(OUT)/obj/%.o)

.PHONY: test test-release test-debug run-tests clean

test:
	@$(MAKE) --no-print-directory test-release
	@$(MAKE) --no-print-directory test-debug

test-release:
	@$(MAKE) --no-print-directory BUILD_MODE=release run-tests

test-debug:
	@$(MAKE) --no-print-directory BUILD_MODE=debug run-tests

run-tests: $(OUT)/test_utils $(OUT)/test_solver
	@echo "== $(BUILD_MODE): test_utils";  $(OUT)/test_utils $(OUT)
	@echo "== $(BUILD_MODE): test_solver"; $(OUT)/test_solver $(OUT)

$(OUT)/obj/utils/%.o: $(FIORE_DIR)/utils/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -MMD -MP -c $< -o $@

$(OUT)/obj/%.o: $(FIORE_DIR)/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(CLINGO_INC) -MMD -MP -c $< -o $@

$(OUT)/test_utils: tests/test_utils.cpp $(UTILS_OBJS)
	$(CXX) $(CXXFLAGS) $^ -o $@

$(OUT)/test_solver: tests/test_solver.cpp $(UTILS_OBJS) $(CLINGO_OBJS)
	$(CXX) $(CXXFLAGS) $(CLINGO_INC) $^ $(CLINGO_LIB) -lclingo -o $@

clean:
	rm -rf build

-include $(UTILS_OBJS:.o=.d) $(CLINGO_OBJS:.o=.d)
