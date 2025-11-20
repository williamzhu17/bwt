# Compiler and flags
CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -O3 -DBUILD_TESTS
LDFLAGS = 

# Source directory
SRC_DIR = ../src
UTIL_DIR = ../util
TEST_DIR = .

# Source files to compile (without main functions)
SRC_FILES = $(SRC_DIR)/bwt.cpp $(SRC_DIR)/inverse_bwt.cpp $(SRC_DIR)/file_processor.cpp
UTIL_FILES = $(UTIL_DIR)/file_utils.cpp $(UTIL_DIR)/format_utils.cpp $(UTIL_DIR)/test_utils.cpp

# Object files
OBJ_FILES = bwt.o inverse_bwt.o file_processor.o file_utils.o format_utils.o test_utils.o

# Find all test .cpp files
TEST_SOURCES = $(wildcard *.cpp)

# Create test executable names in desired order (small tests first)
TEST_EXECUTABLES = test_small test_medium

# Performance benchmark executables
PERF_EXECUTABLES = performance_medium

# All executables
ALL_EXECUTABLES = $(TEST_EXECUTABLES) $(PERF_EXECUTABLES)

# Default target: build all test executables
all: $(ALL_EXECUTABLES)

# Build object files from source files (with BUILD_TESTS defined to exclude main)
$(OBJ_FILES): $(SRC_FILES) $(UTIL_FILES)
	@echo "Building object files from source..."
	@$(CXX) $(CXXFLAGS) -c $(SRC_DIR)/bwt.cpp -o bwt.o
	@$(CXX) $(CXXFLAGS) -c $(SRC_DIR)/inverse_bwt.cpp -o inverse_bwt.o
	@$(CXX) $(CXXFLAGS) -c $(SRC_DIR)/file_processor.cpp -o file_processor.o
	@$(CXX) $(CXXFLAGS) -c $(UTIL_DIR)/file_utils.cpp -o file_utils.o
	@$(CXX) $(CXXFLAGS) -c $(UTIL_DIR)/format_utils.cpp -o format_utils.o
	@$(CXX) $(CXXFLAGS) -c $(UTIL_DIR)/test_utils.cpp -o test_utils.o

# Pattern rule to build test executables
# Each test links against the object files
%: %.cpp $(OBJ_FILES)
	@echo "Building test $@ from $<"
	@$(CXX) $(CXXFLAGS) -I$(SRC_DIR) -I$(UTIL_DIR) $< $(OBJ_FILES) -o $@ $(LDFLAGS)

# Run correctness tests only
test: $(TEST_EXECUTABLES)
	@echo "\n=== Running correctness tests ==="
	@for test in $(TEST_EXECUTABLES); do \
		echo "\n--- Running $$test ---"; \
		./$$test || exit 1; \
	done
	@echo "\n=== All tests passed! ==="

# Run performance benchmarks
performance: $(PERF_EXECUTABLES)
	@echo "\n=== Running performance benchmarks ==="
	@for perf in $(PERF_EXECUTABLES); do \
		echo "\n--- Running $$perf ---"; \
		./$$perf || exit 1; \
	done
	@echo "\n=== Performance benchmarks complete! ==="

# Run a specific test
run-%: %
	@./$<

# Clean target
clean:
	rm -f $(ALL_EXECUTABLES) $(OBJ_FILES)
	rm -rf tmp/

# Rebuild everything
rebuild: clean all

# Phony targets
.PHONY: all test performance clean rebuild run-%

