CC = g++
CFLAGS = -Wall -Wextra -std=c++20 -I/opt/homebrew/include -I$(CONDA_PREFIX)/include $(LOGGER_ACTIVE) -O3 $(EXTRA_CFLAGS)
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
