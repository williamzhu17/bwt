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

Both executables require input and output file arguments. They process files in blocks, which can optionally be configured.

#### `bwt` - Forward Burrows-Wheeler Transform

Applies the BWT transform to an input file and writes the result to an output file.

**Usage:**
```bash
./bwt <input_file> <output_file> [block_size]
```

**Arguments:**
- `input_file` - Path to the input file to transform (required)
- `output_file` - Path where the transformed output will be written (required)
- `block_size` - Size of each block in bytes (optional, default: 128)

**Examples:**
```bash
# From src/ directory with default block size
cd src
./bwt input.txt output.bwt

# From project root with custom block size (64KB blocks)
./src/bwt data/large_text/alice_in_wonderland.txt alice.bwt 65536

# From project root with 1KB blocks
./src/bwt data/large_text/alice_in_wonderland.txt alice.bwt 1024
```

#### `inverse_bwt` - Inverse Burrows-Wheeler Transform

Reverses the BWT transform on an input file and writes the original data to an output file.

**Usage:**
```bash
./inverse_bwt <input_file> <output_file> [block_size]
```

**Arguments:**
- `input_file` - Path to the BWT-transformed input file (required)
- `output_file` - Path where the original (inverted) output will be written (required)
- `block_size` - Size of each block in bytes (optional, default: 128)
  - **Note:** The block size must match the block size used during the forward transform

**Examples:**
```bash
# From src/ directory with default block size
cd src
./inverse_bwt output.bwt restored.txt

# From project root with custom block size (must match forward transform)
./src/inverse_bwt alice.bwt alice_restored.txt 65536

# Complete round-trip example
./src/bwt input.txt transformed.bwt 1024
./src/inverse_bwt transformed.bwt restored.txt 1024
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
