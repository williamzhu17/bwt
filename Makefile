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
BZIP2_INVERSE_BWT_EXTRACTOR := $(BUILD_DIR)/bzip2_inverse_bwt_extractor
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

.PHONY: all clean test performance rebuild bzip2_benchmark

all: $(MAIN_EXECS) $(TEST_EXECS) $(PERF_EXECS) $(BZIP2_BWT_EXTRACTOR) $(BZIP2_INVERSE_BWT_EXTRACTOR) $(BWT_COMPARE)

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

# Build bzip2 inverse BWT extractor (standalone, no bzip2 library needed)
$(BZIP2_INVERSE_BWT_EXTRACTOR): $(TEST_DIR)/bzip2_inverse_bwt_extractor.cpp
	@echo "Building bzip2 inverse BWT extractor $@"
	@mkdir -p $(BUILD_DIR)
	@$(CXX) $(CXXFLAGS) -I$(SRC_DIR) -I$(UTIL_DIR) -c $(TEST_DIR)/bzip2_inverse_bwt_extractor.cpp -o $(BUILD_DIR)/bzip2_inverse_bwt_extractor.o
	@$(CXX) $(CXXFLAGS) $(BUILD_DIR)/bzip2_inverse_bwt_extractor.o -o $@ $(LDFLAGS)

# Build BWT comparison tool (needs bzip2 object files for inline calls)
$(BWT_COMPARE): $(TEST_DIR)/compare_bwt_performance.cpp $(TEST_LIB_OBJS) $(UTIL_OBJS) \
                $(BUILD_DIR)/blocksort.o $(BUILD_DIR)/bzlib.o $(BUILD_DIR)/compress.o \
                $(BUILD_DIR)/decompress.o $(BUILD_DIR)/huffman.o $(BUILD_DIR)/crctable.o $(BUILD_DIR)/randtable.o
	@echo "Building BWT comparison tool $@"
	@mkdir -p $(BUILD_DIR)
	@if [ ! -f $(BUILD_DIR)/blocksort.o ]; then \
		echo "Compiling bzip2 C files..."; \
		$(CC) $(CFLAGS) -I$(TEST_DIR)/../bzip2 -c $(TEST_DIR)/../bzip2/blocksort.c -o $(BUILD_DIR)/blocksort.o; \
		$(CC) $(CFLAGS) -I$(TEST_DIR)/../bzip2 -c $(TEST_DIR)/../bzip2/bzlib.c -o $(BUILD_DIR)/bzlib.o; \
		$(CC) $(CFLAGS) -I$(TEST_DIR)/../bzip2 -c $(TEST_DIR)/../bzip2/compress.c -o $(BUILD_DIR)/compress.o; \
		$(CC) $(CFLAGS) -I$(TEST_DIR)/../bzip2 -c $(TEST_DIR)/../bzip2/decompress.c -o $(BUILD_DIR)/decompress.o; \
		$(CC) $(CFLAGS) -I$(TEST_DIR)/../bzip2 -c $(TEST_DIR)/../bzip2/huffman.c -o $(BUILD_DIR)/huffman.o; \
		$(CC) $(CFLAGS) -I$(TEST_DIR)/../bzip2 -c $(TEST_DIR)/../bzip2/crctable.c -o $(BUILD_DIR)/crctable.o; \
		$(CC) $(CFLAGS) -I$(TEST_DIR)/../bzip2 -c $(TEST_DIR)/../bzip2/randtable.c -o $(BUILD_DIR)/randtable.o; \
	fi
	@$(CXX) $(CXXFLAGS) -DBUILD_TESTS $(INCLUDES) -I$(TEST_DIR)/../bzip2 \
		$(TEST_DIR)/compare_bwt_performance.cpp $(TEST_LIB_OBJS) $(UTIL_OBJS) \
		$(BUILD_DIR)/blocksort.o $(BUILD_DIR)/bzlib.o $(BUILD_DIR)/compress.o \
		$(BUILD_DIR)/decompress.o $(BUILD_DIR)/huffman.o $(BUILD_DIR)/crctable.o $(BUILD_DIR)/randtable.o \
		-o $@ $(LDFLAGS)

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

# BWT Comparison Benchmark
# Usage: make bzip2_benchmark BENCH_DIR=<directory>
# Example: make bzip2_benchmark BENCH_DIR=data/canterbury_corpus
# Note: Uses default block sizes: 64KB, 128KB, 256KB
bzip2_benchmark: $(BWT_COMPARE)
	@if [ -z "$(BENCH_DIR)" ]; then \
		echo "Error: BENCH_DIR not specified"; \
		echo "Usage: make bzip2_benchmark BENCH_DIR=<directory>"; \
		echo "  Example: make bzip2_benchmark BENCH_DIR=data/canterbury_corpus"; \
		echo "  Note: Uses default block sizes: 64KB, 128KB, 256KB"; \
		exit 1; \
	fi
	@if [ ! -d "$(BENCH_DIR)" ]; then \
		echo "Error: Directory '$(BENCH_DIR)' does not exist"; \
		exit 1; \
	fi
	@mkdir -p $(BUILD_DIR)/tmp
	@echo "\n" $(shell printf '=%.0s' {1..80})
	@echo "BWT Performance Comparison: Your Implementation vs bzip2"
	@echo "Directory: $(BENCH_DIR)"
	@echo "Block Sizes: 64KB, 128KB, 256KB (default)"
	@echo "Number of Trials: 5"
	@echo $(shell printf '=%.0s' {1..80})
	@FILE_COUNT=0; \
	SUCCESS_COUNT=0; \
	SUMMARY_FILE=$(BUILD_DIR)/tmp/benchmark_summary.txt; \
	rm -f $$SUMMARY_FILE; \
	for file in $(BENCH_DIR)/*; do \
		if [ -f "$$file" ]; then \
			FILE_COUNT=$$((FILE_COUNT + 1)); \
			echo ""; \
			echo "Testing: $$(basename $$file)"; \
			if ./$(BWT_COMPARE) "$$file" 2>&1 | tee -a $$SUMMARY_FILE; then \
				SUCCESS_COUNT=$$((SUCCESS_COUNT + 1)); \
			fi; \
		fi; \
	done; \
	echo ""; \
	echo $(shell printf '=%.0s' {1..80}); \
	echo "Aggregate Summary"; \
	echo $(shell printf '=%.0s' {1..80}); \
	if [ -f $$SUMMARY_FILE ]; then \
		echo ""; \
		printf "%-30s %-10s %12s %12s %10s %15s %12s\n" "Test File" "Phase" "Your BWT (ms)" "bzip2 (ms)" "Speedup" "Winner" "Faster By"; \
		echo $(shell printf '=%.0s' {1..110}); \
		grep "^SUMMARY|" $$SUMMARY_FILE | while IFS='|' read -r prefix test_name phase your_time bzip2_time speedup winner speedup_pct; do \
			printf "%-30s %-10s %12.3f %12.3f %10.3fx %15s %11.1f%%\n" \
				"$$test_name" "$$phase" "$$your_time" "$$bzip2_time" "$$speedup" "$$winner" "$$speedup_pct"; \
		done; \
		echo ""; \
		YOUR_WINS=$$(grep "^SUMMARY|" $$SUMMARY_FILE | grep -c "your_bwt" || echo 0); \
		BZIP2_WINS=$$(grep "^SUMMARY|" $$SUMMARY_FILE | grep -c "bzip2" || echo 0); \
		TOTAL=$$(grep -c "^SUMMARY|" $$SUMMARY_FILE || echo 0); \
		echo "Summary Statistics:"; \
		echo "  Your BWT faster: $$YOUR_WINS / $$TOTAL"; \
		echo "  bzip2 faster:    $$BZIP2_WINS / $$TOTAL"; \
	fi; \
	echo ""; \
	echo $(shell printf '=%.0s' {1..80}); \
	echo "Benchmark Complete: $$SUCCESS_COUNT/$$FILE_COUNT files tested successfully"; \
	echo $(shell printf '=%.0s' {1..80}); \
	rm -f $$SUMMARY_FILE

clean:
	@echo "Cleaning..."
	@rm -rf $(BUILD_DIR) $(MAIN_EXECS) $(TEST_EXECS) $(PERF_EXECS) $(BZIP2_BWT_EXTRACTOR) $(BZIP2_INVERSE_BWT_EXTRACTOR) $(BWT_COMPARE)
	@rm -rf tests/tmp
	@rm -rf plots
	@rm -f $(BUILD_DIR)/blocksort.o $(BUILD_DIR)/bzlib.o $(BUILD_DIR)/compress.o \
	      $(BUILD_DIR)/decompress.o $(BUILD_DIR)/huffman.o $(BUILD_DIR)/crctable.o \
	      $(BUILD_DIR)/randtable.o $(BUILD_DIR)/bzip2_bwt_extractor.o \
	      $(BUILD_DIR)/bzip2_inverse_bwt_extractor.o

rebuild: clean all

