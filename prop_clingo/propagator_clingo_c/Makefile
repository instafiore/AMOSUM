CC = g++
CFLAGS = -Wall -Wextra --std=c++17 
LDFLAGS =  -lclingo
BUILD_DIR = build
BIN_DIR = bin
INCLUDES =  
SRC_DIR = ../../
TARGET = propagator_clingo
C=cpp

SRCS = $(wildcard $(SRC_DIR)/*.$(C)) # Finds all .c files in the src directory
OBJS = $(patsubst $(SRC_DIR)/%.$(C), $(BUILD_DIR)/%.o, $(SRCS)) # Maps .c to .o in build dir
OBJS += build/propagator_clingo.o # Maps .c to .o in build dir

all: $(TARGET)

test: 
	@echo $(OBJS)

$(TARGET): $(OBJS)
	@echo "Linking $@"
	@mkdir -p $(BIN_DIR)
	$(CC) $(LDFLAGS) -o $(BIN_DIR)/$@ $^

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.$(C)
	@mkdir -p $(BUILD_DIR) # Create build directory if not exists
	@echo "Compiling $< -> $@"
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/propagator_clingo.o: propagator_clingo.$(C)
	$(CC) $(CFLAGS) -c $< -o $@

run: $(TARGET)
	@echo "Running $(TARGET):"
	@./$(TARGET)

clean:
	@echo "Cleaning up..."
	@rm -rf $(BUILD_DIR) $(BIN_DIR)