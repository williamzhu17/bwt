# BWT (Burrows-Wheeler Transform) Project

A C++ implementation of the Burrows-Wheeler Transform and its inverse.

## Building

### Prerequisites
- C++17 compatible compiler (g++, clang++, etc.)
- Make

### Build Instructions

1. Navigate to the `src/` directory:
   ```bash
   cd src
   ```

2. Run make to build all source files:
   ```bash
   make
   ```

3. Executables will be created in the `src/` directory with the same name as their source files (without the `.cpp` extension).

### Other Make Targets

- `make clean` - Remove all built executables from `src/`
- `make rebuild` - Clean and rebuild everything

### Running Executables

After building, run executables from the `src/` directory:
```bash
cd src
./bwt
./inverse_bwt
```

Or from the project root:
```bash
./src/bwt
./src/inverse_bwt
```

## Project Structure
- `src/` - Source files and Makefile (executables are built here)
- `tests/` - Test files
- `data/` - Test data files
