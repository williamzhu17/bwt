# BWT (Burrows-Wheeler Transform) Project

A C++ implementation of the Burrows-Wheeler Transform and its inverse. This project provides tools for transforming files using BWT and recovering them, along with a comprehensive test suite and performance benchmarks.

## Building

### Prerequisites
- C++17 compatible compiler (g++, clang++, etc.)
- Make
- Python 3 (for Python benchmarks)

### Build Instructions

1. Navigate to the project root directory.

2. Run `make` to build all executables (main tools, tests, and benchmarks):
   ```bash
   make
   ```

3. All executables and build artifacts will be created in the `build/` directory.

### Make Targets

- `make` or `make all`: Build everything (main tools, tests, benchmarks).
- `make clean`: Remove the `build/` directory and all artifacts.
- `make rebuild`: Clean and rebuild everything.
- `make test`: Build and run the correctness test suite.
- `make performance`: Build and run the C++ performance benchmarks.

## Usage

The project produces two main executables for file processing.

### `bwt` - Forward Burrows-Wheeler Transform

Applies the BWT transform to an input file and writes the result to an output file.

**Usage:**
```bash
./build/bwt <input_file> <output_file> [block_size]
```

**Arguments:**
- `input_file`: Path to the input file to transform (required).
- `output_file`: Path where the transformed output will be written (required).
- `block_size`: Size of each block in bytes (optional, default: 128).

**Example:**
```bash
# Transform a file using 1KB blocks
./build/bwt data/sample.txt sample.bwt 1024
```

### `inverse_bwt` - Inverse Burrows-Wheeler Transform

Reverses the BWT transform on an input file and writes the original data to an output file.

**Usage:**
```bash
./build/inverse_bwt <input_file> <output_file> [block_size]
```

**Arguments:**
- `input_file`: Path to the BWT-transformed input file (required).
- `output_file`: Path where the original (recovered) output will be written (required).
- `block_size`: Size of each block in bytes (optional, default: 128).
  - **Note:** The block size must match the block size used during the forward transform.

**Example:**
```bash
# Recover the original file
./build/inverse_bwt sample.bwt recovered.txt 1024
```

## Testing

The project includes a comprehensive testing infrastructure to verify correctness and measure performance.

### Running Tests

To run the full correctness test suite:
```bash
make test
```

This runs two sets of tests:
1.  **Small Tests**: Unit tests for basic functionality, edge cases, and small strings.
2.  **Medium Tests**: Integration tests that run round-trip conversions (Forward BWT -> Inverse BWT) on real files from the Canterbury Corpus. It verifies that the recovered file is byte-for-byte identical to the original.

### Running Performance Benchmarks

To run C++ performance benchmarks:
```bash
make performance
```

This runs the `performance_medium` suite, which measures:
- Forward BWT execution time.
- Inverse BWT execution time.
- Total round-trip time.
- Throughput (MB/s).

### Additional Scripts

- `tests/performance_medium.py`: A Python implementation benchmark that mirrors the C++ performance test.
  ```bash
  python3 tests/performance_medium.py
  ```
- `test_roundtrip_performance.sh`: A shell script to compare C++ and Python implementations on a specific file.
  ```bash
  ./test_roundtrip_performance.sh data/canterbury_corpus/alice29.txt
  ```

### Test Artifacts

- **Executables**: Test executables are located in `build/` (e.g., `build/test_small`, `build/test_medium`).
- **Temporary Files**: Intermediate files created during testing (e.g., `.bwt` files, recovered files) are stored in `build/tmp/`.

## Project Structure

- `src/`: Source code for the main application and BWT logic.
- `tests/`: Source code for test suites and benchmarks.
- `util/`: Utility helpers for file I/O and testing.
- `data/`: Test data files (Canterbury Corpus, etc.).
- `build/`: Generated directory containing compiled executables and temporary test output.
- `Makefile`: Root build configuration.

## Credits

This project uses test data from the **Canterbury Corpus**, a collection of standard benchmark files for evaluating compression algorithms.
http://corpus.canterbury.ac.nz/
This project also uses test data from **Silesia Corpus**, a comprehensive compression benchmark corpus containing diverse file types representative of real-world data. Maintained by the Silesian University of Technology. 
http://sun.aei.polsl.pl/~sdeor/index.php?page=silesia
