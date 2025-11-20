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

## Testing

### Running Tests

1. Navigate to the `tests/` directory:
   ```bash
   cd tests
   ```

2. Build and run all tests:
   ```bash
   make test
   ```

   This will:
   - Build all test executables
   - Run all tests
   - Display a summary of passed/failed tests

### Other Test Make Targets

- `make` or `make all` - Build all test executables
- `make test_small` - Build only the `test_small` executable
- `make run-test_small` - Run a specific test (e.g., `test_small`)
- `make clean` - Remove all test executables and object files
- `make rebuild` - Clean and rebuild everything

### Running Tests from Project Root

You can also run tests from the project root:
```bash
cd tests
make test_small
./tests/test_small
```

### Test Output

Tests will display:
- Individual test results with `[PASS]` or `[FAIL]` status
- Error messages for failed tests
- A summary showing total tests, passed, and failed counts

## Project Structure
- `src/` - Source files and Makefile (executables are built here)
- `tests/` - Test files
- `data/` - Test data files
