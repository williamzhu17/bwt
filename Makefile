# Compiler and flags
CXX := g++
CXXFLAGS := -std=c++17 -Wall -Wextra -O3
LDFLAGS := 

# Directories
SRC_DIR := src
TEST_DIR := tests
UTIL_DIR := util
BUILD_DIR := build

# Include directories
INCLUDES := -I$(SRC_DIR) -I$(UTIL_DIR)

# Main Executables
BWT_EXEC := $(BUILD_DIR)/bwt
INV_BWT_EXEC := $(BUILD_DIR)/inverse_bwt
MAIN_EXECS := $(BWT_EXEC) $(INV_BWT_EXEC)

# Test Executables
TEST_EXECS := $(BUILD_DIR)/test_small $(BUILD_DIR)/test_medium
PERF_EXECS := $(BUILD_DIR)/performance

# Source Files
UTIL_SRCS := $(wildcard $(UTIL_DIR)/*.cpp)

# --- Object Definitions ---

# 1. Production Objects (for main executables)
# Compiled directly from src/ without extra macros
PROD_OBJS := $(BUILD_DIR)/src/file_processor.o

# 2. Test Library Objects (for test executables)
# Compiled from src/ but with -DBUILD_TESTS to suppress main()
TEST_LIB_OBJS := $(BUILD_DIR)/src_test/bwt.o \
                 $(BUILD_DIR)/src_test/inverse_bwt.o \
                 $(BUILD_DIR)/src_test/file_processor.o

# 3. Utility Objects (for test executables)
UTIL_OBJS := $(patsubst $(UTIL_DIR)/%.cpp,$(BUILD_DIR)/util/%.o,$(UTIL_SRCS))

# --- Main Targets ---

.PHONY: all clean test performance rebuild

all: $(MAIN_EXECS) $(TEST_EXECS) $(PERF_EXECS)

# Build Main Executables
$(BWT_EXEC): $(BUILD_DIR)/src/bwt.o $(PROD_OBJS)
	@echo "Building $@"
	@$(CXX) $(CXXFLAGS) $^ -o $@ $(LDFLAGS)

$(INV_BWT_EXEC): $(BUILD_DIR)/src/inverse_bwt.o $(PROD_OBJS)
	@echo "Building $@"
	@$(CXX) $(CXXFLAGS) $^ -o $@ $(LDFLAGS)

# Build Test Executables
$(TEST_EXECS): $(BUILD_DIR)/%: $(TEST_DIR)/%.cpp $(TEST_LIB_OBJS) $(UTIL_OBJS)
	@echo "Building test $@"
	@$(CXX) $(CXXFLAGS) -DBUILD_TESTS $(INCLUDES) $^ -o $@ $(LDFLAGS)

$(PERF_EXECS): $(BUILD_DIR)/%: $(TEST_DIR)/%.cpp $(TEST_LIB_OBJS) $(UTIL_OBJS)
	@echo "Building benchmark $@"
	@$(CXX) $(CXXFLAGS) -DBUILD_TESTS $(INCLUDES) $^ -o $@ $(LDFLAGS)

# --- Compilation Rules ---

# Compile src files for Production
$(BUILD_DIR)/src/%.o: $(SRC_DIR)/%.cpp
	@mkdir -p $(dir $@)
	@echo "Compiling $< (prod)"
	@$(CXX) $(CXXFLAGS) -c $< -o $@

# Compile src files for Tests (with -DBUILD_TESTS)
$(BUILD_DIR)/src_test/%.o: $(SRC_DIR)/%.cpp
	@mkdir -p $(dir $@)
	@echo "Compiling $< (test mode)"
	@$(CXX) $(CXXFLAGS) -DBUILD_TESTS -c $< -o $@

# Compile util files
$(BUILD_DIR)/util/%.o: $(UTIL_DIR)/%.cpp
	@mkdir -p $(dir $@)
	@echo "Compiling $<"
	@$(CXX) $(CXXFLAGS) -DBUILD_TESTS -c $< -o $@

# --- Runners ---

test: $(TEST_EXECS)
	@echo "\n=== Running Correctness Tests ==="
	@for t in $(TEST_EXECS); do \
		echo "\n--- $$t ---"; \
		./$$t || exit 1; \
	done
	@echo "\n=== All Tests Passed! ==="

performance: $(PERF_EXECS)
	@echo "\n=== Running Performance Benchmarks ==="
	@for t in $(PERF_EXECS); do \
		echo "\n--- $$t ---"; \
		./$$t || exit 1; \
	done

clean:
	@echo "Cleaning..."
	@rm -rf $(BUILD_DIR) $(MAIN_EXECS) $(TEST_EXECS) $(PERF_EXECS)
	@rm -rf tests/tmp

rebuild: clean all

