CC = g++
CFLAGS = -Wall -Wextra -std=c++17 -I/opt/homebrew/include $(DEBUG) $(CHECK_MPS) $(SANITIZE_ADDRESS)  -O3
LDFLAGS = -L/opt/homebrew/lib -lclingo $(SANITIZE_ADDRESS)
BUILD_DIR = build
BIN_DIR = bin
INCLUDES =  
SRC_DIR = ../..
TARGET = runner_clingo
C=cpp
PROP_WASP = $(SRC_DIR)/prop_wasp/propagator_wasp_c

SRCS = $(shell find $(SRC_DIR) -name '*.$(C)')
OBJS = $(addprefix $(BUILD_DIR)/, $(notdir $(SRCS:.$(C)=.o)))

all: $(TARGET)

test:
	@echo $(SRCS)
	@echo $(OBJS)

$(TARGET): $(OBJS)
	@echo "Linking $@"
	@mkdir -p $(BIN_DIR)
	$(CC) $^ $(LDFLAGS) -o $(BIN_DIR)/$@

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.$(C)
	@mkdir -p $(BUILD_DIR) # Create build directory if not exists
	@echo "Compiling $< -> $@"
	$(CC) $(CFLAGS) $(LDFLAGS) -c $< -o $@

$(BUILD_DIR)/%.o: $(PROP_WASP)/%.$(C)
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
