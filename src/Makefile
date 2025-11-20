# Compiler and flags
CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -O3
LDFLAGS = 

# Executables with main() functions
EXECUTABLES = bwt inverse_bwt

# Object files that are shared/library code
SHARED_OBJS = file_processor.o

# Default target: build all executables
all: $(EXECUTABLES)

# Build bwt executable (depends on file_processor)
bwt: bwt.cpp file_processor.o
	@echo "Building $@ from $<"
	$(CXX) $(CXXFLAGS) bwt.cpp file_processor.o -o $@ $(LDFLAGS)

# Build inverse_bwt executable (depends on file_processor)
inverse_bwt: inverse_bwt.cpp file_processor.o
	@echo "Building $@ from $<"
	$(CXX) $(CXXFLAGS) inverse_bwt.cpp file_processor.o -o $@ $(LDFLAGS)

# Build file_processor object file
file_processor.o: file_processor.cpp file_processor.hpp
	@echo "Building object file $@"
	$(CXX) $(CXXFLAGS) -c file_processor.cpp -o $@

# Clean target
clean:
	rm -f $(EXECUTABLES) $(SHARED_OBJS)

# Rebuild everything
rebuild: clean all

# Phony targets
.PHONY: all clean rebuild

