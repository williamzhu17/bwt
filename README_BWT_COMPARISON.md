# Comparing Your BWT Implementation with bzip2

This guide explains how to compare the performance of your BWT implementation with bzip2's BWT implementation.

## Overview

bzip2 includes a complete compression pipeline:
1. **BWT (Burrows-Wheeler Transform)** - implemented in `blocksort.c`
2. **MTF (Move-To-Front)** - implemented in `compress.c`
3. **Huffman Coding** - implemented in `huffman.c`

To compare just the BWT performance, we've created tools that extract only the BWT step from bzip2.

## Building the Comparison Tools

1. **Build the bzip2 BWT extractor:**
   ```bash
   make build/bzip2_bwt_extractor
   ```
   
   This compiles a standalone program that uses bzip2's `BZ2_blockSort` function to perform only the BWT transform (without MTF or Huffman coding).

2. **Build the comparison tool:**
   ```bash
   make build/compare_bwt_performance
   ```
   
   This compiles a program that runs both implementations and compares their performance.

## Usage

### Option 1: Direct Comparison Tool

Run the comparison tool on a test file:

```bash
./build/compare_bwt_performance <input_file> [block_size]
```

Example:
```bash
./build/compare_bwt_performance data/canterbury_corpus/alice29.txt 65536
```

This will:
- Run your BWT implementation
- Run bzip2's BWT implementation
- Compare execution times, throughput, and output sizes
- Display a detailed comparison report

### Option 2: Manual Comparison

1. **Run your BWT:**
   ```bash
   ./build/bwt input.txt output_your.bwt 65536
   ```

2. **Run bzip2 BWT extractor:**
   ```bash
   ./build/bzip2_bwt_extractor input.txt output_bzip2.bwt 65536
   ```

3. **Compare the results manually** (check file sizes, verify correctness, measure times)

## Understanding the Output

The comparison tool reports:
- **Time**: Execution time in milliseconds for each implementation
- **Output Size**: Size of the BWT-transformed output
- **Speedup**: Ratio of execution times (if > 1, bzip2 is faster)
- **Throughput**: Processing speed in MB/s

## Technical Details

### bzip2's BWT Implementation

bzip2 uses a sophisticated sorting algorithm:
- **For small blocks (< 10KB)**: Uses a fallback O(N log²N) algorithm
- **For larger blocks**: Uses a radix-sort based algorithm with:
  - Initial radix sort on 2-byte prefixes
  - Main sort using a combination of quicksort and pointer scanning
  - Quadrant caching for repetitive blocks

The main function is `BZ2_blockSort()` in `blocksort.c`, which:
- Takes an `EState` structure containing the block data
- Sorts the block using suffix array construction
- Stores the sorted order in `s->ptr[]`
- Stores the original pointer in `s->origPtr`

### Your BWT Implementation

Your implementation uses:
- **Suffix array construction**: Prefix-doubling algorithm
- **Multi-threaded processing**: Processes blocks in parallel
- **Block-based processing**: Handles large files by splitting into blocks

### Key Differences

1. **Algorithm**: 
   - bzip2: Radix-sort based with optimizations
   - Yours: Prefix-doubling suffix array construction

2. **Threading**:
   - bzip2: Single-threaded (per block)
   - Yours: Multi-threaded across blocks

3. **Block Size**:
   - bzip2: Fixed at 100KB-900KB (configurable via blockSize100k)
   - Yours: Configurable block size

## Notes

### Output Format Differences

**Your BWT Implementation:**
- Writes a delimiter byte (first byte of file) - a unique byte not in the input
- Each BWT chunk includes the delimiter in the transformed output
- Format: `[delimiter_byte][BWT_chunk_1][BWT_chunk_2]...`
- Each chunk size = block_size + 1 (includes delimiter)

**bzip2 BWT Extractor:**
- Writes a marker byte (0xFF) followed by the original pointer (3 bytes)
- Each BWT chunk does NOT include a delimiter
- Format: `[0xFF][origPtr_3bytes][BWT_chunk_1][0xFF][origPtr_3bytes][BWT_chunk_2]...`
- Each chunk size = block_size (no delimiter)

**Important:** The output formats are NOT compatible. The bzip2 extractor output cannot be used with your inverse BWT implementation. However, the BWT transform itself (the permutation of characters) is equivalent - both implementations perform the same mathematical transform.

### Performance Considerations

Performance may vary significantly based on:
- **Input file characteristics**: Repetitive data favors bzip2's algorithm, random data may favor yours
- **Block size**: Different algorithms have different optimal block sizes
- **System resources**: Your multi-threaded implementation benefits from multiple CPU cores
- **Memory access patterns**: Cache behavior can significantly impact performance

## Troubleshooting

**Error: bzip2_bwt_extractor not found**
- Make sure you've built it: `make build/bzip2_bwt_extractor`
- Check that all bzip2 source files are present in the `bzip2/` directory

**Compilation errors**
- Ensure you have a C++ compiler that supports C++17
- Check that all bzip2 source files are accessible
- Some systems may need additional flags (e.g., `-fPIC` for shared libraries)

**Different output sizes**
- This is expected! The bzip2 extractor uses a different output format
- The BWT transform itself should be equivalent, but metadata differs

## Further Analysis

To get more detailed performance data:
1. Run multiple trials and average the results
2. Test with different block sizes
3. Test with different file types (text, binary, repetitive, random)
4. Profile both implementations to identify bottlenecks

