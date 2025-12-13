#!/usr/bin/env python3
"""
Test script to demonstrate linear scaling O(n) of BWT algorithm.

This script:
1. Generates test files of varying sizes
2. Measures execution time for forward and inverse BWT
3. Creates plots showing time vs input size (should be linear)
4. Verifies linear scaling by showing constant time/n ratio
"""

import sys
import os
import time
import subprocess
import argparse
import statistics
import numpy as np
import matplotlib.pyplot as plt
from pathlib import Path
import tempfile
import shutil
import random

# Configuration
DEFAULT_NUM_TRIALS = 5
DEFAULT_BLOCK_SIZE = 65536  # 64KB blocks
BWT_EXEC = "build/bwt"
INV_BWT_EXEC = "build/inverse_bwt"


def generate_test_file(size_bytes, output_path, pattern="random"):
    """
    Generate a test file of specified size.
    
    IMPORTANT: Ensures byte value 0 is never used, so there's always
    a unique delimiter available for BWT.
    
    Args:
        size_bytes: Target file size in bytes
        output_path: Path to write the file
        pattern: Type of pattern to generate ('random', 'repetitive', 'text')
    """
    if pattern == "random":
        # Generate random bytes, but exclude byte 0 to ensure unique delimiter
        # Use bytes in range 1-255 (255 possible values, byte 0 reserved as delimiter)
        data = bytes([random.randint(1, 255) for _ in range(size_bytes)])
    elif pattern == "repetitive":
        # Generate repetitive pattern (good for BWT)
        # Uses ASCII chars 'A'-'H' (0x41-0x48), all > 0, so safe
        pattern_str = b"ABCDEFGH" * (size_bytes // 8 + 1)
        data = pattern_str[:size_bytes]
    elif pattern == "text":
        # Generate text-like pattern
        # Uses printable ASCII and whitespace, all > 0, so safe
        chars = b"abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789 \n\t"
        data = b"".join([bytes([chars[i % len(chars)]]) for i in range(size_bytes)])
    else:
        raise ValueError(f"Unknown pattern: {pattern}")
    
    with open(output_path, 'wb') as f:
        f.write(data)


def measure_bwt_time(input_file, output_file, block_size, is_forward=True):
    """
    Measure execution time for BWT transform.
    
    Args:
        input_file: Path to input file
        output_file: Path to output file
        block_size: Block size to use
        is_forward: True for forward BWT, False for inverse BWT
    
    Returns:
        Execution time in seconds, or None if failed
    """
    exec_path = BWT_EXEC if is_forward else INV_BWT_EXEC
    exec_path = os.path.join(Path(__file__).parent.parent, exec_path)
    
    if not os.path.exists(exec_path):
        print(f"Error: Executable not found: {exec_path}", file=sys.stderr)
        return None
    
    # Clean up output file if it exists
    if os.path.exists(output_file):
        os.remove(output_file)
    
    try:
        start_time = time.perf_counter()
        result = subprocess.run(
            [exec_path, input_file, output_file, str(block_size)],
            capture_output=True,
            text=True,
            timeout=300  # 5 minute timeout
        )
        end_time = time.perf_counter()
        
        if result.returncode != 0:
            print(f"Error running {exec_path}: {result.stderr}", file=sys.stderr)
            return None
        
        if not os.path.exists(output_file):
            print(f"Error: Output file not created: {output_file}", file=sys.stderr)
            return None
        
        return end_time - start_time
    
    except subprocess.TimeoutExpired:
        print(f"Error: {exec_path} timed out", file=sys.stderr)
        return None
    except Exception as e:
        print(f"Error running {exec_path}: {e}", file=sys.stderr)
        return None


def run_scaling_test(sizes, block_size, num_trials, pattern="random", temp_dir=None):
    """
    Run scaling tests for multiple file sizes.
    
    Args:
        sizes: List of file sizes in bytes to test
        block_size: Block size to use for BWT
        num_trials: Number of trials per size
        pattern: Pattern type for test files
        temp_dir: Temporary directory for test files
    
    Returns:
        Dictionary with results: {size: {'forward': [times], 'inverse': [times]}}
    """
    if temp_dir is None:
        temp_dir = tempfile.mkdtemp(prefix="bwt_scaling_test_")
    else:
        os.makedirs(temp_dir, exist_ok=True)
    
    results = {}
    
    try:
        for size in sizes:
            print(f"Testing size: {size:,} bytes ({size / 1024:.2f} KB)")
            results[size] = {
                'forward': [],
                'inverse': []
            }
            
            input_file = os.path.join(temp_dir, f"input_{size}.dat")
            forward_file = os.path.join(temp_dir, f"forward_{size}.bwt")
            recovered_file = os.path.join(temp_dir, f"recovered_{size}.dat")
            
            # Generate test file
            print(f"  Generating test file...")
            generate_test_file(size, input_file, pattern)
            
            # Run forward BWT trials
            print(f"  Running forward BWT ({num_trials} trials)...")
            for trial in range(num_trials):
                elapsed = measure_bwt_time(input_file, forward_file, block_size, is_forward=True)
                if elapsed is not None:
                    results[size]['forward'].append(elapsed)
                else:
                    print(f"    Trial {trial + 1} failed", file=sys.stderr)
            
            # Run inverse BWT trials
            if results[size]['forward']:  # Only if forward succeeded
                print(f"  Running inverse BWT ({num_trials} trials)...")
                for trial in range(num_trials):
                    elapsed = measure_bwt_time(forward_file, recovered_file, block_size, is_forward=False)
                    if elapsed is not None:
                        results[size]['inverse'].append(elapsed)
                    else:
                        print(f"    Trial {trial + 1} failed", file=sys.stderr)
            
            # Clean up intermediate files
            for f in [input_file, forward_file, recovered_file]:
                if os.path.exists(f):
                    os.remove(f)
            
            print(f"  Forward: {len(results[size]['forward'])}/{num_trials} successful")
            print(f"  Inverse: {len(results[size]['inverse'])}/{num_trials} successful")
            print()
    
    finally:
        # Clean up temp directory
        if os.path.exists(temp_dir) and temp_dir.startswith(tempfile.gettempdir()):
            shutil.rmtree(temp_dir)
    
    return results


def create_scaling_plots(results, output_dir="plots_scaling"):
    """
    Create plots demonstrating linear scaling.
    
    Args:
        results: Results dictionary from run_scaling_test
        output_dir: Directory to save plots
    """
    os.makedirs(output_dir, exist_ok=True)
    
    # Extract data
    sizes = sorted([s for s in results.keys() if results[s]['forward'] and results[s]['inverse']])
    
    if len(sizes) < 2:
        print("Error: Need at least 2 successful test sizes to create plots", file=sys.stderr)
        return
    
    forward_means = [statistics.mean(results[s]['forward']) for s in sizes]
    inverse_means = [statistics.mean(results[s]['inverse']) for s in sizes]
    total_means = [f + i for f, i in zip(forward_means, inverse_means)]
    
    forward_stds = [statistics.stdev(results[s]['forward']) if len(results[s]['forward']) > 1 else 0 
                   for s in sizes]
    inverse_stds = [statistics.stdev(results[s]['inverse']) if len(results[s]['inverse']) > 1 else 0 
                   for s in sizes]
    
    sizes_kb = [s / 1024 for s in sizes]
    sizes_mb = [s / (1024 * 1024) for s in sizes]
    
    # Plot 1: Time vs Input Size (linear scale) - shows linear relationship
    fig, ax = plt.subplots(figsize=(10, 6))
    
    ax.plot(sizes_kb, forward_means, 'o-', label='Forward BWT', markersize=6)
    ax.plot(sizes_kb, inverse_means, 's-', label='Inverse BWT', markersize=6)
    ax.plot(sizes_kb, total_means, 'D-', label='Roundtrip (Forward + Inverse)', markersize=6)
    
    ax.set_xlabel('Input Size (KB)', fontsize=12)
    ax.set_ylabel('Execution Time (seconds)', fontsize=12)
    ax.set_title('BWT Algorithm Scaling: Time vs Input Size', fontsize=14, fontweight='bold')
    ax.legend(loc='best', fontsize=10)
    ax.grid(True, alpha=0.3)
    
    plt.tight_layout()
    plt.savefig(os.path.join(output_dir, 'time_vs_size_linear.png'), dpi=300, bbox_inches='tight')
    print(f"Saved: {os.path.join(output_dir, 'time_vs_size_linear.png')}")
    plt.close()
    
    # Plot 2: Log-log plot - slope should be ~1 for linear scaling
    fig, ax = plt.subplots(figsize=(10, 6))
    
    ax.loglog(sizes, forward_means, 'o-', label='Forward BWT', base=2, markersize=6)
    ax.loglog(sizes, inverse_means, 's-', label='Inverse BWT', base=2, markersize=6)
    ax.loglog(sizes, total_means, 'D-', label='Roundtrip', base=2, markersize=6)
    
    # Fit power law: time = a * size^b, log(time) = log(a) + b * log(size)
    # For linear scaling, b should be ~1
    for label, times in [('Forward', forward_means), ('Inverse', inverse_means), ('Roundtrip', total_means)]:
        log_sizes = np.log2(sizes)
        log_times = np.log2(times)
        coeffs = np.polyfit(log_sizes, log_times, 1)
        exponent = coeffs[0]
        intercept = coeffs[1]
        
        # Generate fit line
        fit_log_times = intercept + exponent * log_sizes
        fit_times = 2 ** fit_log_times
        
        ax.loglog(sizes, fit_times, '--', alpha=0.5, 
                 label=f'{label} fit (exponent={exponent:.3f})', base=2)
    
    ax.set_xlabel('Input Size (bytes)', fontsize=12)
    ax.set_ylabel('Execution Time (seconds)', fontsize=12)
    ax.set_title('BWT Algorithm Scaling: Log-Log Plot (Linear Scaling → Slope ≈ 1)', 
                fontsize=14, fontweight='bold')
    ax.legend(loc='best', fontsize=10)
    ax.grid(True, alpha=0.3, which='both')
    
    plt.tight_layout()
    plt.savefig(os.path.join(output_dir, 'time_vs_size_loglog.png'), dpi=300, bbox_inches='tight')
    print(f"Saved: {os.path.join(output_dir, 'time_vs_size_loglog.png')}")
    plt.close()
    
    # Plot 3: Time per Byte (Time/n) vs Size - should be constant for linear scaling
    fig, ax = plt.subplots(figsize=(10, 6))
    
    forward_time_per_byte = [t / s for t, s in zip(forward_means, sizes)]
    inverse_time_per_byte = [t / s for t, s in zip(inverse_means, sizes)]
    total_time_per_byte = [t / s for t, s in zip(total_means, sizes)]
    
    ax.plot(sizes_kb, [t * 1e9 for t in forward_time_per_byte], 'o-', label='Forward BWT (ns/byte)', markersize=6)
    ax.plot(sizes_kb, [t * 1e9 for t in inverse_time_per_byte], 's-', label='Inverse BWT (ns/byte)', markersize=6)
    ax.plot(sizes_kb, [t * 1e9 for t in total_time_per_byte], 'D-', label='Roundtrip (ns/byte)', markersize=6)
    
    # Calculate average time per byte
    avg_forward = statistics.mean(forward_time_per_byte)
    avg_inverse = statistics.mean(inverse_time_per_byte)
    avg_total = statistics.mean(total_time_per_byte)
    
    ax.axhline(y=avg_forward * 1e9, color='blue', linestyle='--', alpha=0.5, 
              label=f'Forward avg: {avg_forward*1e9:.2f} ns/byte')
    ax.axhline(y=avg_inverse * 1e9, color='orange', linestyle='--', alpha=0.5, 
              label=f'Inverse avg: {avg_inverse*1e9:.2f} ns/byte')
    ax.axhline(y=avg_total * 1e9, color='green', linestyle='--', alpha=0.5, 
              label=f'Roundtrip avg: {avg_total*1e9:.2f} ns/byte')
    
    ax.set_xlabel('Input Size (KB)', fontsize=12)
    ax.set_ylabel('Time per Byte (nanoseconds)', fontsize=12)
    ax.set_title('BWT Algorithm Scaling: Time per Byte vs Input Size\n(Constant = Linear O(n) Scaling)', 
                fontsize=14, fontweight='bold')
    ax.legend(loc='best', fontsize=10)
    ax.grid(True, alpha=0.3)
    
    plt.tight_layout()
    plt.savefig(os.path.join(output_dir, 'time_per_byte_vs_size.png'), dpi=300, bbox_inches='tight')
    print(f"Saved: {os.path.join(output_dir, 'time_per_byte_vs_size.png')}")
    plt.close()
    
    # Plot 4: Throughput (MB/s) vs Size - should be constant for linear scaling
    fig, ax = plt.subplots(figsize=(10, 6))
    
    forward_throughput = [(s / (1024 * 1024)) / t for s, t in zip(sizes, forward_means)]
    inverse_throughput = [(s / (1024 * 1024)) / t for s, t in zip(sizes, inverse_means)]
    total_throughput = [(s / (1024 * 1024)) / t for s, t in zip(sizes, total_means)]
    
    ax.plot(sizes_mb, forward_throughput, 'o-', label='Forward BWT', markersize=6)
    ax.plot(sizes_mb, inverse_throughput, 's-', label='Inverse BWT', markersize=6)
    ax.plot(sizes_mb, total_throughput, 'D-', label='Roundtrip', markersize=6)
    
    ax.set_xlabel('Input Size (MB)', fontsize=12)
    ax.set_ylabel('Throughput (MB/s)', fontsize=12)
    ax.set_title('BWT Algorithm Throughput vs Input Size\n(Constant Throughput = Linear O(n) Scaling)', 
                fontsize=14, fontweight='bold')
    ax.legend(loc='best', fontsize=10)
    ax.grid(True, alpha=0.3)
    
    plt.tight_layout()
    plt.savefig(os.path.join(output_dir, 'throughput_vs_size.png'), dpi=300, bbox_inches='tight')
    print(f"Saved: {os.path.join(output_dir, 'throughput_vs_size.png')}")
    plt.close()
    
    # Print summary statistics
    print("\n" + "=" * 70)
    print("Scaling Analysis Summary")
    print("=" * 70)
    print(f"{'Size (KB)':<12} {'Forward (s)':<15} {'Inverse (s)':<15} {'Total (s)':<15} {'Throughput (MB/s)':<18}")
    print("-" * 70)
    
    for size, f_mean, i_mean, t_mean, tput in zip(sizes, forward_means, inverse_means, 
                                                    total_means, total_throughput):
        print(f"{size/1024:<12.2f} {f_mean:<15.6f} {i_mean:<15.6f} {t_mean:<15.6f} {tput:<18.2f}")
    
    print("\nLinear Scaling Verification:")
    print(f"Average time per byte (Forward):  {avg_forward*1e9:.2f} ns/byte")
    print(f"Average time per byte (Inverse):  {avg_inverse*1e9:.2f} ns/byte")
    print(f"Average time per byte (Roundtrip): {avg_total*1e9:.2f} ns/byte")
    print("\nNote: Constant time per byte indicates O(n) linear scaling")
    print("=" * 70)


def main():
    parser = argparse.ArgumentParser(
        description='Test BWT algorithm linear scaling O(n)',
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
  # Test with default sizes (1KB to 16MB)
  python3 tests/test_linear_scaling.py
  
  # Test with custom size range
  python3 tests/test_linear_scaling.py --min-size 1024 --max-size 8388608 --sizes 10
  
  # Test with more trials for better accuracy
  python3 tests/test_linear_scaling.py --trials 10
        """
    )
    
    parser.add_argument('--min-size', type=int, default=1024,
                       help='Minimum file size in bytes (default: 1024 = 1KB)')
    parser.add_argument('--max-size', type=int, default=16*1024*1024,
                       help='Maximum file size in bytes (default: 16MB)')
    parser.add_argument('--sizes', type=int, default=8,
                       help='Number of sizes to test (default: 8)')
    parser.add_argument('--block-size', type=int, default=DEFAULT_BLOCK_SIZE,
                       help=f'Block size in bytes (default: {DEFAULT_BLOCK_SIZE})')
    parser.add_argument('--trials', type=int, default=DEFAULT_NUM_TRIALS,
                       help=f'Number of trials per size (default: {DEFAULT_NUM_TRIALS})')
    parser.add_argument('--pattern', choices=['random', 'repetitive', 'text'], 
                       default='random',
                       help='Pattern type for test files (default: random)')
    parser.add_argument('--output-dir', default='plots_scaling',
                       help='Output directory for plots (default: plots_scaling)')
    parser.add_argument('--temp-dir',
                       help='Temporary directory for test files (default: system temp)')
    
    args = parser.parse_args()
    
    # Generate size list (logarithmically spaced)
    if args.sizes == 1:
        sizes = [args.min_size]
    else:
        sizes = [int(x) for x in np.logspace(
            np.log2(args.min_size), 
            np.log2(args.max_size), 
            args.sizes, 
            base=2
        )]
    
    print("=" * 70)
    print("BWT Algorithm Linear Scaling Test")
    print("=" * 70)
    print(f"Size range: {args.min_size:,} bytes ({args.min_size/1024:.2f} KB) to "
          f"{args.max_size:,} bytes ({args.max_size/(1024*1024):.2f} MB)")
    print(f"Number of sizes: {len(sizes)}")
    print(f"Block size: {args.block_size:,} bytes ({args.block_size/1024:.2f} KB)")
    print(f"Trials per size: {args.trials}")
    print(f"Pattern: {args.pattern}")
    print(f"Sizes to test: {[f'{s:,}' for s in sizes]}")
    print("=" * 70)
    print()
    
    # Run tests
    results = run_scaling_test(
        sizes=sizes,
        block_size=args.block_size,
        num_trials=args.trials,
        pattern=args.pattern,
        temp_dir=args.temp_dir
    )
    
    # Create plots
    print("\nGenerating plots...")
    create_scaling_plots(results, output_dir=args.output_dir)
    
    print("\nTest complete!")
    return 0


if __name__ == "__main__":
    sys.exit(main())

