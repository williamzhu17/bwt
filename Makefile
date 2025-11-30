# Compiler and flags
CXX := g++
PYTHON := python3
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

# BWT Comparison Tools
BZIP2_BWT_EXTRACTOR := $(BUILD_DIR)/bzip2_bwt_extractor
BWT_COMPARE := $(BUILD_DIR)/compare_bwt_performance

# Source Files
# Note: Exclude template implementation files that are included from headers
UTIL_SRCS := $(filter-out $(UTIL_DIR)/blocking_queue.cpp $(UTIL_DIR)/reorder_buffer.cpp, $(wildcard $(UTIL_DIR)/*.cpp))

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

all: $(MAIN_EXECS) $(TEST_EXECS) $(PERF_EXECS) $(BZIP2_BWT_EXTRACTOR) $(BWT_COMPARE)

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

# Build bzip2 BWT extractor (requires bzip2 source files)
# Note: Compiles C and C++ files together
# We need to compile C files without C++-specific flags
CC := gcc
CFLAGS := -Wall -Wextra -O3

$(BZIP2_BWT_EXTRACTOR): $(TEST_DIR)/bzip2_bwt_extractor.cpp
	@echo "Building bzip2 BWT extractor $@"
	@mkdir -p $(BUILD_DIR)
	@echo "Compiling C files..."
	@$(CC) $(CFLAGS) -I$(TEST_DIR)/../bzip2 -c $(TEST_DIR)/../bzip2/blocksort.c -o $(BUILD_DIR)/blocksort.o
	@$(CC) $(CFLAGS) -I$(TEST_DIR)/../bzip2 -c $(TEST_DIR)/../bzip2/bzlib.c -o $(BUILD_DIR)/bzlib.o
	@$(CC) $(CFLAGS) -I$(TEST_DIR)/../bzip2 -c $(TEST_DIR)/../bzip2/compress.c -o $(BUILD_DIR)/compress.o
	@$(CC) $(CFLAGS) -I$(TEST_DIR)/../bzip2 -c $(TEST_DIR)/../bzip2/decompress.c -o $(BUILD_DIR)/decompress.o
	@$(CC) $(CFLAGS) -I$(TEST_DIR)/../bzip2 -c $(TEST_DIR)/../bzip2/huffman.c -o $(BUILD_DIR)/huffman.o
	@$(CC) $(CFLAGS) -I$(TEST_DIR)/../bzip2 -c $(TEST_DIR)/../bzip2/crctable.c -o $(BUILD_DIR)/crctable.o
	@$(CC) $(CFLAGS) -I$(TEST_DIR)/../bzip2 -c $(TEST_DIR)/../bzip2/randtable.c -o $(BUILD_DIR)/randtable.o
	@echo "Compiling C++ file and linking..."
	@$(CXX) $(CXXFLAGS) -I$(TEST_DIR)/../bzip2 -I$(SRC_DIR) -I$(UTIL_DIR) \
		-c $(TEST_DIR)/bzip2_bwt_extractor.cpp -o $(BUILD_DIR)/bzip2_bwt_extractor.o
	@$(CXX) $(CXXFLAGS) $(BUILD_DIR)/bzip2_bwt_extractor.o \
		$(BUILD_DIR)/blocksort.o $(BUILD_DIR)/bzlib.o $(BUILD_DIR)/compress.o \
		$(BUILD_DIR)/decompress.o $(BUILD_DIR)/huffman.o $(BUILD_DIR)/crctable.o $(BUILD_DIR)/randtable.o \
		-o $@ $(LDFLAGS)

# Build BWT comparison tool
$(BWT_COMPARE): $(TEST_DIR)/compare_bwt_performance.cpp $(TEST_LIB_OBJS) $(UTIL_OBJS)
	@echo "Building BWT comparison tool $@"
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

python_performance:
	@echo "\n=== Running Python Performance Benchmarks ==="
	@echo "\n--- tests/performance.py ---"
	@$(PYTHON) -u tests/performance.py || exit 1

clean:
	@echo "Cleaning..."
	@rm -rf $(BUILD_DIR) $(MAIN_EXECS) $(TEST_EXECS) $(PERF_EXECS) $(BZIP2_BWT_EXTRACTOR) $(BWT_COMPARE)
	@rm -rf tests/tmp
	@rm -rf plots
	@rm -f $(BUILD_DIR)/blocksort.o $(BUILD_DIR)/bzlib.o $(BUILD_DIR)/compress.o \
	      $(BUILD_DIR)/decompress.o $(BUILD_DIR)/huffman.o $(BUILD_DIR)/crctable.o \
	      $(BUILD_DIR)/randtable.o $(BUILD_DIR)/bzip2_bwt_extractor.o

rebuild: clean all

