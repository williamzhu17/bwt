# Compiler and flags
CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -O3
LDFLAGS = 

# Find all .cpp files in current directory
SOURCES = $(wildcard *.cpp)

# Create executable names (remove .cpp extension) - same directory as source files
EXECUTABLES = $(SOURCES:%.cpp=%)

# Default target: build all executables
# Use -k flag to continue building even if one fails
all:
	@$(MAKE) -k $(EXECUTABLES) || true

# Pattern rule to build executables from .cpp files
# Note: Only files with main() function can be compiled as executables
%: %.cpp
	@echo "Building $@ from $<"
	@$(CXX) $(CXXFLAGS) $< -o $@ $(LDFLAGS) 2>&1 || echo "Warning: $@ failed to build (may not have main() function)"

# Clean target
clean:
	rm -f $(EXECUTABLES)

# Rebuild everything
rebuild: clean all

# Phony targets
.PHONY: all clean rebuild

