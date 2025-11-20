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

### Test Infrastructure

The project includes a comprehensive testing infrastructure with two main test suites:

#### `test_small`
- Tests basic BWT and inverse BWT functionality
- Uses small, hardcoded test strings
- Validates correctness on edge cases and simple examples
- Fast execution for quick validation during development

#### `test_medium`
- Tests real-world files with various sizes and content types
- Dynamically generates test cases from all files in a specified directory
- Tests multiple block sizes (128, 256, 512, 1024, 4096, 16384 bytes)
- Performs full round-trip validation (forward BWT → inverse BWT)
- Saves intermediate files to `tests/tmp/` for inspection and debugging
- Verifies byte-by-byte identity between original and recovered files
- Reports detailed statistics including file sizes and test counts

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

3. Run individual test suites:
   ```bash
   # Run small tests (quick validation)
   make run-test_small
   
   # Run medium tests (comprehensive file testing)
   make run-test_medium
   ```

### Other Test Make Targets

- `make` or `make all` - Build all test executables
- `make test_small` - Build only the `test_small` executable
- `make test_medium` - Build only the `test_medium` executable
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
- Error messages for failed tests including:
  - File size mismatches
  - Round-trip validation failures
  - File I/O errors
- A summary showing total tests, passed, and failed counts

### Test Data

The project uses several test data sets:

- **Canterbury Corpus** (`data/canterbury_corpus/`) - Standard compression benchmark files including text, source code, HTML, binary data, and more
- **Silesia Corpus** (`data/silesia/`) - Large-scale compression benchmark files with diverse content types
- **Large Files** (`data/large_size/`) - Additional large test files such as the King James Bible

### Inspecting Test Outputs

When running medium tests, all intermediate files are saved to `tests/tmp/`:
- Files ending in `__forward` contain the BWT-transformed data
- Files ending in `__recovered` contain the data after inverse BWT
- These files can be inspected to debug transformation issues or verify correctness

## Project Structure
- `src/` - Source files and Makefile (executables are built here)
- `tests/` - Test files and test infrastructure
- `data/` - Test data files
  - `canterbury_corpus/` - Standard compression benchmark files
  - `silesia/` - Large-scale compression benchmark files
  - `large_size/` - Additional large test files

## Credits and Acknowledgments

This project uses test data from the following sources:

- **Canterbury Corpus**: A collection of standard benchmark files for evaluating compression algorithms. Created by the University of Canterbury and widely used in compression research. Available at: http://corpus.canterbury.ac.nz/

- **Silesia Corpus**: A comprehensive compression benchmark corpus containing diverse file types representative of real-world data. Maintained by the Silesian University of Technology. Available at: http://sun.aei.polsl.pl/~sdeor/index.php?page=silesia

These corpora provide valuable test cases with varying characteristics (text, binary, structured data, etc.) to ensure robust BWT implementation across different data types.
