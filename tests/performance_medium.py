#!/usr/bin/env python3
"""
Performance benchmarking script for Python BWT implementation
Tests all files in Canterbury Corpus with multiple trials
"""

import sys
import os
import time
import argparse
import statistics
from pathlib import Path

# Add parent directory to path to import bwt modules
sys.path.insert(0, str(Path(__file__).parent.parent / 'src'))

from bwt import forward as bwt_forward
from inverse_bwt import inverse as bwt_inverse


# Configuration
NUM_TRIALS = 5  # Default number of trials


class PerformanceMetrics:
    """Store and calculate performance metrics"""
    
    def __init__(self):
        self.forward_times = []
        self.inverse_times = []
        self.total_times = []
        self.input_size = 0
        self.output_size = 0
        
        # Statistics (calculated later)
        self.forward_mean = 0
        self.forward_stddev = 0
        self.inverse_mean = 0
        self.inverse_stddev = 0
        self.total_mean = 0
        self.total_stddev = 0
    
    def calculate_statistics(self):
        """Calculate mean and standard deviation for all metrics"""
        if self.forward_times:
            self.forward_mean = statistics.mean(self.forward_times)
            self.forward_stddev = statistics.stdev(self.forward_times) if len(self.forward_times) > 1 else 0
        
        if self.inverse_times:
            self.inverse_mean = statistics.mean(self.inverse_times)
            self.inverse_stddev = statistics.stdev(self.inverse_times) if len(self.inverse_times) > 1 else 0
        
        if self.total_times:
            self.total_mean = statistics.mean(self.total_times)
            self.total_stddev = statistics.stdev(self.total_times) if len(self.total_times) > 1 else 0


def create_directory(dir_path):
    """Create directory if it doesn't exist"""
    try:
        os.makedirs(dir_path, exist_ok=True)
        return True
    except OSError:
        return False


def get_file_size(filename):
    """Get file size in bytes"""
    try:
        return os.path.getsize(filename)
    except OSError:
        return 0


def run_performance_test(input_file, block_size, num_trials):
    """Run performance test with multiple trials"""
    metrics = PerformanceMetrics()
    metrics.input_size = get_file_size(input_file)
    
    # Temporary file names
    forward_file = "tmp/perf_forward.bwt"
    recovered_file = "tmp/perf_recovered.txt"
    
    for trial in range(num_trials):
        # Clean up previous files
        for f in [forward_file, recovered_file]:
            if os.path.exists(f):
                os.remove(f)
        
        # Forward BWT with timing
        forward_start = time.perf_counter()
        try:
            with open(input_file, 'rb') as infile, open(forward_file, 'wb') as outfile:
                while True:
                    chunk = infile.read(block_size)
                    if len(chunk) == 0:
                        break
                    
                    # Convert bytes to string, apply BWT, convert back
                    chunk_str = chunk.decode('latin-1')
                    result = bwt_forward(chunk_str)
                    outfile.write(result.encode('latin-1'))
        except Exception as e:
            print(f"Error in forward BWT: {e}", file=sys.stderr)
            return metrics
        
        forward_end = time.perf_counter()
        forward_duration = forward_end - forward_start
        metrics.forward_times.append(forward_duration)
        
        # Get output size (only once)
        if trial == 0:
            metrics.output_size = get_file_size(forward_file)
        
        # Inverse BWT with timing
        inverse_start = time.perf_counter()
        try:
            bwt_chunk_size = block_size + 1  # Account for delimiter
            with open(forward_file, 'rb') as infile, open(recovered_file, 'wb') as outfile:
                while True:
                    chunk = infile.read(bwt_chunk_size)
                    if len(chunk) == 0:
                        break
                    
                    # Convert bytes to string, apply inverse BWT, convert back
                    chunk_str = chunk.decode('latin-1')
                    result = bwt_inverse(chunk_str)
                    outfile.write(result.encode('latin-1'))
        except Exception as e:
            print(f"Error in inverse BWT: {e}", file=sys.stderr)
            return metrics
        
        inverse_end = time.perf_counter()
        inverse_duration = inverse_end - inverse_start
        metrics.inverse_times.append(inverse_duration)
        
        # Record total time
        metrics.total_times.append(forward_duration + inverse_duration)
    
    # Clean up temporary files
    for f in [forward_file, recovered_file]:
        if os.path.exists(f):
            os.remove(f)
    
    # Calculate statistics
    metrics.calculate_statistics()
    
    return metrics


def format_time(seconds):
    """Format time with appropriate units"""
    if seconds < 0.001:
        return f"{seconds * 1000000:.4f} μs"
    elif seconds < 1.0:
        return f"{seconds * 1000:.4f} ms"
    else:
        return f"{seconds:.4f} s"


def format_size(bytes_val):
    """Format file size with appropriate units"""
    if bytes_val < 1024:
        return f"{bytes_val} B"
    elif bytes_val < 1024 * 1024:
        return f"{bytes_val / 1024:.2f} KB"
    else:
        return f"{bytes_val / (1024 * 1024):.2f} MB"


def print_performance_results(test_name, metrics, block_size):
    """Print formatted performance results"""
    print("\n" + "=" * 70)
    print(f"Test: {test_name}")
    print(f"Block Size: {format_size(block_size)}")
    print(f"Input Size: {format_size(metrics.input_size)}")
    print(f"Output Size: {format_size(metrics.output_size)}")
    
    compression_ratio = metrics.output_size / metrics.input_size if metrics.input_size > 0 else 0
    print(f"Compression Ratio: {compression_ratio:.4f}")
    
    print("-" * 70)
    print(f"Trials: {len(metrics.forward_times)}")
    print("-" * 70)
    
    # Forward BWT results
    print("Forward BWT:")
    print(f"  Mean:   {format_time(metrics.forward_mean)}", end="")
    if len(metrics.forward_times) > 1:
        print(f" ± {format_time(metrics.forward_stddev)}")
    else:
        print()
    
    print(f"  Min:    {format_time(min(metrics.forward_times))}")
    print(f"  Max:    {format_time(max(metrics.forward_times))}")
    
    # Inverse BWT results
    print("\nInverse BWT:")
    print(f"  Mean:   {format_time(metrics.inverse_mean)}", end="")
    if len(metrics.inverse_times) > 1:
        print(f" ± {format_time(metrics.inverse_stddev)}")
    else:
        print()
    
    print(f"  Min:    {format_time(min(metrics.inverse_times))}")
    print(f"  Max:    {format_time(max(metrics.inverse_times))}")
    
    # Total roundtrip results
    print("\nTotal Roundtrip:")
    print(f"  Mean:   {format_time(metrics.total_mean)}", end="")
    if len(metrics.total_times) > 1:
        print(f" ± {format_time(metrics.total_stddev)}")
    else:
        print()
    
    print(f"  Min:    {format_time(min(metrics.total_times))}")
    print(f"  Max:    {format_time(max(metrics.total_times))}")
    
    # Throughput
    throughput_mb_s = (metrics.input_size / (1024 * 1024)) / metrics.total_mean if metrics.total_mean > 0 else 0
    print(f"\nThroughput: {throughput_mb_s:.2f} MB/s")
    
    print("=" * 70)


def list_files_in_directory(dir_path):
    """List all regular files in a directory"""
    try:
        files = []
        for entry in os.listdir(dir_path):
            full_path = os.path.join(dir_path, entry)
            if os.path.isfile(full_path):
                files.append(entry)
        return sorted(files)
    except OSError as e:
        print(f"Warning: Could not open directory: {dir_path}: {e}", file=sys.stderr)
        return []


def generate_test_cases(data_dir, block_sizes):
    """Generate test cases for all files with all block sizes"""
    test_cases = []
    files = list_files_in_directory(data_dir)
    
    for filename in files:
        for block_size in block_sizes:
            test_cases.append({
                'name': filename,
                'input_file': os.path.join(data_dir, filename),
                'block_size': block_size
            })
    
    return test_cases


def main():
    parser = argparse.ArgumentParser(
        description='Performance benchmark for Python BWT implementation'
    )
    parser.add_argument(
        'num_trials',
        nargs='?',
        type=int,
        default=NUM_TRIALS,
        help=f'Number of trials per test (default: {NUM_TRIALS})'
    )
    
    args = parser.parse_args()
    
    num_trials = args.num_trials
    if num_trials < 1:
        print(f"Invalid number of trials. Using default: {NUM_TRIALS}", file=sys.stderr)
        num_trials = NUM_TRIALS
    
    print("\n" + "=" * 70)
    print("BWT Performance Benchmark - Canterbury Corpus (Python)")
    print("=" * 70)
    print(f"Number of trials per test: {num_trials}")
    
    # Create tmp directory for temporary files
    if not create_directory("tmp"):
        print("Error: Failed to create tmp directory", file=sys.stderr)
        return 1
    
    # Define data directory and block sizes to test
    data_dir = "../data/canterbury_corpus"
    block_sizes = [128]  # Start with 128, can add more: [128, 256, 512, 1024]
    
    # Check if data directory exists
    if not os.path.isdir(data_dir):
        print(f"Error: Data directory not found: {data_dir}", file=sys.stderr)
        return 1
    
    # Generate test cases
    print(f"Scanning directory: {data_dir}")
    test_cases = generate_test_cases(data_dir, block_sizes)
    
    if not test_cases:
        print("Error: No test cases generated. Check if data directory contains files.", file=sys.stderr)
        return 1
    
    num_files = len(test_cases) // len(block_sizes)
    print(f"Found {num_files} files")
    print(f"Testing {len(block_sizes)} block sizes")
    print(f"Total test cases: {len(test_cases)}")
    print("=" * 70)
    
    # Run all performance tests
    completed = 0
    for test_case in test_cases:
        completed += 1
        print(f"\n[{completed}/{len(test_cases)}] Running: {test_case['name']} "
              f"(block size: {format_size(test_case['block_size'])})")
        
        # Check if input file exists
        if not os.path.isfile(test_case['input_file']):
            print(f"Error: Input file not found: {test_case['input_file']}", file=sys.stderr)
            continue
        
        # Run performance test
        metrics = run_performance_test(
            test_case['input_file'],
            test_case['block_size'],
            num_trials
        )
        
        # Print results
        print_performance_results(test_case['name'], metrics, test_case['block_size'])
    
    print("\n" + "=" * 70)
    print("Performance Benchmark Complete!")
    print(f"Total tests completed: {completed}")
    print("=" * 70)
    
    return 0


if __name__ == "__main__":
    sys.exit(main())

