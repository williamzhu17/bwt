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
- `make python_performance`: Run the Python performance benchmarks.

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

This runs the benchmark suite on the default **Canterbury Corpus**. The benchmark measures:
- Forward BWT execution time.
- Inverse BWT execution time.
- Total round-trip time.
- Throughput (MB/s).

You can also run the benchmark on a custom dataset:
```bash
./build/performance <dataset_path> [num_trials]
```

**Arguments:**
- `dataset_path`: Path to the directory containing test files.
- `num_trials`: Number of times to run each test (optional, default: 5).

**Example:**
```bash
# Run on Silesia corpus with 2 trials per file
./build/performance data/silesia 2
```

### Comparing with bzip2

To compare your BWT implementation's performance with bzip2's BWT:

```bash
make bzip2_benchmark BENCH_DIR=<directory>
```

**Arguments:**
- `BENCH_DIR`: Directory containing test files (required).

**Example:**
```bash
# Compare on Canterbury Corpus
make bzip2_benchmark BENCH_DIR=data/canterbury_corpus
```

**Note:** The benchmark automatically tests multiple default block sizes (64KB, 128KB, and 256KB) to provide comprehensive performance comparisons. The bzip2 implementation automatically allocates sufficient space to handle these sizes.

This benchmark performs a comprehensive comparison across three phases:
- **Forward BWT**: Compares forward transform performance
- **Inverse BWT**: Compares inverse transform performance
- **Round Trip**: Compares total time for forward + inverse transform

For each phase, the benchmark:
- Runs 5 trials per file for statistical accuracy
- Tests multiple block sizes (64KB, 128KB, 256KB) automatically
- Compares execution time, throughput, and output sizes
- Calculates speedup ratios and statistical measures (mean, stddev, min, max)
- Displays a detailed comparison report and aggregate summary
- **Uses inline function calls** for both implementations to ensure fair comparison (no process overhead)

You can also run the comparison tool directly on a single file:
```bash
./build/compare_bwt_performance <input_file>
```

This will automatically test forward BWT, inverse BWT, and round trip performance for the specified file across all default block sizes (64KB, 128KB, 256KB).

### Additional Scripts

- `make python_performance`: Runs the Python implementation benchmark (`tests/performance.py`) which mirrors the C++ performance test.
  ```bash
  make python_performance
  # OR
  python3 tests/performance.py [data_dir] [num_trials]
  ```
- `test_roundtrip_performance.sh`: A shell script to compare C++ and Python implementations on a specific file.
  ```bash
  ./test_roundtrip_performance.sh data/canterbury_corpus/alice29.txt
  ```

### Generating Performance Plots

You can visualize the performance results by generating bar charts comparing C++ and Python implementations.

1. **Prerequisites**: Install Python dependencies.
   ```bash
   # It is recommended to use a virtual environment
   python3 -m venv venv
   source venv/bin/activate
   pip install -r util/requirements.txt
   ```

2. **Run the plotting script**:
   The script requires both the C++ and Python benchmark log files.
   ```bash
   python3 util/plot_performance.py --cpp <cpp_log_file> --python <python_log_file> [--output <output_dir>]
   ```

   **Example:**
   ```bash
   # Generate plots from recent logs
   python3 util/plot_performance.py --cpp 11-20-cpp-performance.log --python 11-20-python-performance.log
   ```

   The plots will be saved in the `plots/` directory by default.

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
