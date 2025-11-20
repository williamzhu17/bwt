# Compiler and flags
CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -DBUILD_TESTS
LDFLAGS = 

# Source directory
SRC_DIR = ../src
TEST_DIR = .

# Source files to compile (without main functions)
SRC_FILES = $(SRC_DIR)/bwt.cpp $(SRC_DIR)/inverse_bwt.cpp $(SRC_DIR)/file_processor.cpp

# Object files
OBJ_FILES = bwt.o inverse_bwt.o file_processor.o

# Find all test .cpp files
TEST_SOURCES = $(wildcard *.cpp)

# Create test executable names (remove .cpp extension)
TEST_EXECUTABLES = $(TEST_SOURCES:%.cpp=%)

# Default target: build all test executables
all: $(TEST_EXECUTABLES)

# Build object files from source files (with BUILD_TESTS defined to exclude main)
$(OBJ_FILES): $(SRC_FILES)
	@echo "Building object files from source..."
	@$(CXX) $(CXXFLAGS) -c $(SRC_DIR)/bwt.cpp -o bwt.o
	@$(CXX) $(CXXFLAGS) -c $(SRC_DIR)/inverse_bwt.cpp -o inverse_bwt.o
	@$(CXX) $(CXXFLAGS) -c $(SRC_DIR)/file_processor.cpp -o file_processor.o

# Pattern rule to build test executables
# Each test links against the object files
%: %.cpp $(OBJ_FILES)
	@echo "Building test $@ from $<"
	@$(CXX) $(CXXFLAGS) -I$(SRC_DIR) $< $(OBJ_FILES) -o $@ $(LDFLAGS)

# Run all tests
test: all
	@echo "\n=== Running all tests ==="
	@for test in $(TEST_EXECUTABLES); do \
		echo "\n--- Running $$test ---"; \
		./$$test || exit 1; \
	done
	@echo "\n=== All tests passed! ==="

# Run a specific test
run-%: %
	@./$<

# Clean target
clean:
	rm -f $(TEST_EXECUTABLES) $(OBJ_FILES)

# Rebuild everything
rebuild: clean all

# Phony targets
.PHONY: all test clean rebuild run-%

