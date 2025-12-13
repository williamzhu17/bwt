#!/usr/bin/env python3
"""
Parse a roundtrip performance log (e.g. logs/kjv_roundtrip_performance.log)
and generate a plot comparing C++ and Python implementations with log scale
for better readability across wide range of values.

Usage:
    python util/plot_roundtrip_log.py --log logs/kjv_roundtrip_performance.log --current_cpp_log logs/kjv_current_cpp_performance.log --output plots
"""

import argparse
import os
from collections import defaultdict

import matplotlib.pyplot as plt


def parse_log(log_path):
    """
    Parse a roundtrip performance log produced by test_roundtrip_performance.sh.

    Returns:
        dict mapping block_size_bytes -> {
            'cpp': {'forward': float, 'inverse': float, 'total': float},
            'py':  {'forward': float, 'inverse': float, 'total': float}
        }
    """
    results = defaultdict(dict)

    if not os.path.isfile(log_path):
        raise FileNotFoundError(f"Log file not found: {log_path}")

    current_impl = None
    current_block_size = None
    forward_time = None
    inverse_time = None
    total_time = None
    prev_line = ""

    with open(log_path, "r", encoding="utf-8") as f:
        for line in f:
            line = line.strip()
            if not line:
                continue

            # Implementation line
            if line.startswith("Implementation:"):
                # If we were in the middle of a block, store it
                if (
                    current_impl is not None
                    and current_block_size is not None
                    and forward_time is not None
                    and inverse_time is not None
                    and total_time is not None
                ):
                    impl_key = "cpp" if current_impl == "cpp" else "py"
                    results[current_block_size][impl_key] = {
                        "forward": forward_time,
                        "inverse": inverse_time,
                        "total": total_time,
                    }

                # Start a new block
                current_impl = line.split(":", 1)[1].strip()
                current_block_size = None
                forward_time = None
                inverse_time = None
                total_time = None

            elif line.startswith("Block Size:"):
                # Format: "Block Size: 64 bytes"
                parts = line.split(":", 1)[1].strip().split()
                # first token is integer number of bytes
                current_block_size = int(parts[0])

            elif line.startswith("Wall time:") and "Forward BWT" in prev_line:
                # "  Wall time: .857015000s"
                time_str = line.split(":", 1)[1].strip()
                if time_str.endswith("s"):
                    time_str = time_str[:-1]
                forward_time = float(time_str)

            elif line.startswith("Wall time:") and "Inverse BWT" in prev_line:
                time_str = line.split(":", 1)[1].strip()
                if time_str.endswith("s"):
                    time_str = time_str[:-1]
                inverse_time = float(time_str)

            elif line.startswith("Total roundtrip time:"):
                # "Total roundtrip time: 13.790250000s"
                time_str = line.split(":", 1)[1].strip()
                if time_str.endswith("s"):
                    time_str = time_str[:-1]
                total_time = float(time_str)

            prev_line = line

    # Flush last block
    if (
        current_impl is not None
        and current_block_size is not None
        and forward_time is not None
        and inverse_time is not None
        and total_time is not None
    ):
        impl_key = "cpp" if current_impl == "cpp" else "py"
        results[current_block_size][impl_key] = {
            "forward": forward_time,
            "inverse": inverse_time,
            "total": total_time,
        }

    return dict(sorted(results.items(), key=lambda kv: kv[0]))


def parse_current_cpp_detailed(log_path):
    """
    Parse the detailed per-block statistics from the C++ performance benchmark output.
    Extracts forward and inverse times for each block size.

    Returns:
        dict mapping block_size_bytes -> {
            'forward': float (seconds),
            'inverse': float (seconds),
            'total': float (seconds)
        }
    """
    if not os.path.isfile(log_path):
        raise FileNotFoundError(f"Current C++ log file not found: {log_path}")

    results = {}
    current_block_size = None
    forward_time = None
    inverse_time = None
    total_time = None
    current_section = None

    with open(log_path, "r", encoding="utf-8") as f:
        lines = f.readlines()
        i = 0
        while i < len(lines):
            stripped = lines[i].strip()
            if not stripped:
                i += 1
                continue

            # Block size line: "Block Size: 64 B"
            if stripped.startswith("Block Size:"):
                # Save previous block if complete
                if current_block_size is not None and forward_time is not None and inverse_time is not None:
                    results[current_block_size] = {
                        'forward': forward_time,
                        'inverse': inverse_time,
                        'total': total_time if total_time is not None else forward_time + inverse_time
                    }
                
                # Parse new block size
                parts = stripped.split(":", 1)[1].strip().split()
                size_val = float(parts[0])
                size_unit = parts[1]
                if size_unit == "B":
                    current_block_size = int(size_val)
                elif size_unit == "KB":
                    current_block_size = int(size_val * 1024)
                else:
                    current_block_size = None
                
                forward_time = None
                inverse_time = None
                total_time = None
                current_section = None

            # Detect section headers
            elif stripped.startswith("Forward BWT:"):
                current_section = "forward"
            elif stripped.startswith("Inverse BWT:"):
                current_section = "inverse"
            elif stripped.startswith("Total Roundtrip:"):
                current_section = "total"

            # Parse Mean line
            elif stripped.startswith("Mean:"):
                parts = stripped.split()
                if len(parts) >= 3:
                    time_val = float(parts[1])
                    time_unit = parts[2]
                    
                    # Convert to seconds
                    if time_unit == "ms":
                        time_seconds = time_val / 1000.0
                    elif time_unit == "s":
                        time_seconds = time_val
                    elif time_unit == "μs" or time_unit == "us":
                        time_seconds = time_val / 1000000.0
                    else:
                        time_seconds = time_val / 1000.0  # Default to ms
                    
                    if current_section == "forward":
                        forward_time = time_seconds
                    elif current_section == "inverse":
                        inverse_time = time_seconds
                    elif current_section == "total":
                        total_time = time_seconds

            i += 1

    # Flush last block
    if current_block_size is not None and forward_time is not None and inverse_time is not None:
        results[current_block_size] = {
            'forward': forward_time,
            'inverse': inverse_time,
            'total': total_time if total_time is not None else forward_time + inverse_time
        }

    return results


def plot_times(results, current_cpp, output_dir, phase="forward", title_base=None):
    """
    Plot forward or inverse BWT time vs block size for:
      - Current C++ (from detailed performance log)
      - Naive C++ (from roundtrip script log)
      - Python
    
    Args:
        phase: "forward" or "inverse"
    """
    os.makedirs(output_dir, exist_ok=True)

    # Use all block sizes that appear in either log, but drop 2048-byte blocks
    all_block_sizes = sorted(set(results.keys()) | set(current_cpp.keys()))
    all_block_sizes = [bs for bs in all_block_sizes if bs != 2048]
    labels = [str(bs) for bs in all_block_sizes]

    current_cpp_times = []
    naive_cpp_times = []
    py_times = []

    for bs in all_block_sizes:
        # Current C++ from detailed performance log
        current_entry = current_cpp.get(bs, {})
        current_cpp_times.append(current_entry.get(phase, float("nan")))

        # Naive C++ and Python from roundtrip log
        entry = results.get(bs, {})
        naive_cpp_times.append(entry.get("cpp", {}).get(phase, float("nan")))
        py_times.append(entry.get("py", {}).get(phase, float("nan")))

    x = list(range(len(all_block_sizes)))
    width = 0.25

    fig, ax = plt.subplots(figsize=(12, 7))
    
    # Plot all three implementations on the same axis with log scale
    color_current = 'tab:blue'
    color_naive = 'tab:orange'
    color_py = 'tab:green'
    
    bars_current = ax.bar([i - width for i in x], current_cpp_times, width, 
                          label="Current C++", color=color_current, alpha=0.8)
    bars_naive = ax.bar(x, naive_cpp_times, width, 
                       label="Naive C++", color=color_naive, alpha=0.8)
    bars_py = ax.bar([i + width for i in x], py_times, width, 
                    label="Python", color=color_py, alpha=0.8)
    
    # Set log scale on y-axis
    ax.set_yscale('log')
    ax.set_xlabel("Block size (bytes)", fontsize=11)
    phase_label = phase.capitalize()
    ax.set_ylabel(f"{phase_label} BWT time (s)", fontsize=11)
    ax.set_xticks(x)
    ax.set_xticklabels(labels)
    ax.grid(True, axis="y", linestyle="--", alpha=0.3, which='both')
    ax.legend(loc='upper left', fontsize=10)

    # Add value labels on top of each bar
    def _label_bars(bars, color):
        for b in bars:
            height = b.get_height()
            if height != height:  # NaN check
                continue
            ax.text(
                b.get_x() + b.get_width() / 2.0,
                height,
                f"{height:.3f}",
                ha="center",
                va="bottom",
                fontsize=8,
                color=color,
                weight='bold'
            )

    _label_bars(bars_current, color_current)
    _label_bars(bars_naive, color_naive)
    _label_bars(bars_py, color_py)

    if title_base:
        plt.title(f"{title_base} - {phase_label} BWT", fontsize=13, pad=20)
    else:
        plt.title(f"BWT {phase_label} Time vs Block Size (Log Scale)", fontsize=13, pad=20)

    out_path = os.path.join(output_dir, f"bwt_{phase}_current_vs_naive_cpp_vs_python.png")
    plt.tight_layout()
    plt.savefig(out_path, dpi=150)
    print(f"Saved plot to {out_path}")


def main():
    parser = argparse.ArgumentParser(
        description="Plot Current C++ vs Naive C++ vs Python forward and inverse BWT performance with log scale"
    )
    parser.add_argument(
        "--log",
        required=True,
        help="Path to a roundtrip performance log (e.g. logs/kjv_roundtrip_performance.log)",
    )
    parser.add_argument(
        "--current_cpp_log",
        required=True,
        help="Path to a C++ performance log containing the aggregate statistics table",
    )
    parser.add_argument(
        "--output",
        default="plots",
        help="Output directory for the generated plot (default: plots)",
    )
    parser.add_argument(
        "--title",
        default=None,
        help="Optional plot title",
    )

    args = parser.parse_args()

    results = parse_log(args.log)
    if not results:
        raise SystemExit(f"No data parsed from log: {args.log}")

    current_cpp = parse_current_cpp_detailed(args.current_cpp_log)
    if not current_cpp:
        raise SystemExit(f"No detailed data parsed from current C++ log: {args.current_cpp_log}")

    # Generate two separate plots: forward and inverse
    if args.title:
        title_base = args.title.replace(" Roundtrip", "").replace(": Roundtrip", "")
    else:
        title_base = None
    plot_times(results, current_cpp, args.output, phase="forward", title_base=title_base)
    plot_times(results, current_cpp, args.output, phase="inverse", title_base=title_base)


if __name__ == "__main__":
    main()

