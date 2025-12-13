#!/usr/bin/env python3
"""
Plot performance comparison: block size vs roundtrip time
for silesia and canterbury datasets.
"""

import os
import re
import argparse
import matplotlib.pyplot as plt


def parse_size_to_kb(size_str: str) -> float:
    """
    Parse strings like '512 B', '1.00 KB', '64.00 KB', '1.00 MB' into KB.
    """
    parts = size_str.strip().split()
    if len(parts) != 2:
        return 0.0
    val = float(parts[0])
    unit = parts[1]
    if unit == "B":
        return val / 1024.0
    if unit == "KB":
        return val
    if unit == "MB":
        return val * 1024.0
    return val


def parse_time_to_seconds(time_str: str) -> float:
    """
    Parse strings like '7.4544 s', '162.6376 ms' into seconds.
    """
    parts = time_str.strip().split()
    if len(parts) != 2:
        return 0.0
    val = float(parts[0])
    unit = parts[1]
    if unit == "s":
        return val
    if unit == "ms":
        return val / 1000.0
    if unit == "μs" or unit == "us":
        return val / 1000000.0
    return val


def parse_per_file_results(filepath: str):
    """
    Parse individual test results from the log file.
    Returns a list of dicts with 'file', 'block_size_kb', 'block_size_label', and 'roundtrip_time_s'.
    """
    records = []
    
    current_file = None
    current_block_size_kb = None
    current_block_size_label = None
    
    with open(filepath, "r", encoding="utf-8") as f:
        lines = f.readlines()
    
    i = 0
    while i < len(lines):
        line = lines[i].strip()
        
        # Parse test name
        if line.startswith("Test:"):
            current_file = line.split(":", 1)[1].strip()
        
        # Parse block size
        elif line.startswith("Block Size:"):
            block_size_str = line.split(":", 1)[1].strip()
            current_block_size_kb = parse_size_to_kb(block_size_str)
            current_block_size_label = block_size_str
        
        # Parse roundtrip time
        elif line.startswith("Total Roundtrip:"):
            # Look ahead for Mean line
            j = i + 1
            while j < len(lines) and j < i + 5:
                subline = lines[j].strip()
                if subline.startswith("Mean:"):
                    # Extract time from "Mean:   965.5483 ms ± 41.5190 ms"
                    time_part = subline.split("Mean:")[1].split("±")[0].strip()
                    roundtrip_time_s = parse_time_to_seconds(time_part)
                    
                    if current_file and current_block_size_kb is not None:
                        records.append({
                            'file': current_file,
                            'block_size_kb': current_block_size_kb,
                            'block_size_label': current_block_size_label,
                            'roundtrip_time_s': roundtrip_time_s
                        })
                    break
                j += 1
        
        i += 1
    
    return records


def parse_aggregate_statistics(filepath: str):
    """
    Parse the aggregate statistics section from the log file.
    Returns a list of dicts with 'block_size_kb' and 'roundtrip_time_s'.
    """
    records = []
    
    with open(filepath, "r", encoding="utf-8") as f:
        content = f.read()
    
    # Find the "Aggregate Statistics by Block Size" section
    start_marker = "Aggregate Statistics by Block Size"
    if start_marker not in content:
        return records
    
    # Find the start of the data section (after the header line and separator)
    start_idx = content.find(start_marker)
    if start_idx == -1:
        return records
    
    # Find the header line and the separator line after it
    section = content[start_idx:]
    lines = section.split('\n')
    
    # Find the header line (contains "Block Size" and "Total Time")
    header_line_idx = -1
    for i, line in enumerate(lines):
        if "Block Size" in line and "Total Time" in line:
            header_line_idx = i
            break
    
    if header_line_idx == -1:
        return records
    
    # Process lines after the header and separator
    for i in range(header_line_idx + 2, len(lines)):
        line = lines[i].strip()
        
        # Stop at the end marker (double equals)
        if line.startswith("==") and len(line) > 10:
            break
        
        # Skip empty lines and separator lines
        if not line or line.startswith("-") or (line.startswith("=") and len(line) > 5):
            continue
        
        # Parse data line
        # Format: "512 B       86.33 MB       7.4544 s       11.58 MB/s        1.0020"
        # Split by whitespace
        parts = line.split()
        if len(parts) >= 6:
            try:
                # Block size: parts[0] + parts[1] (e.g., "512 B" or "1.00 KB")
                block_size_str = f"{parts[0]} {parts[1]}"
                # Total time: parts[4] + parts[5] (e.g., "7.4544 s" or "162.6376 ms")
                total_time_str = f"{parts[4]} {parts[5]}"
                
                block_size_kb = parse_size_to_kb(block_size_str)
                roundtrip_time_s = parse_time_to_seconds(total_time_str)
                
                records.append({
                    'block_size_kb': block_size_kb,
                    'block_size_label': block_size_str,
                    'roundtrip_time_s': roundtrip_time_s
                })
            except (ValueError, IndexError) as e:
                # Skip malformed lines
                continue
    
    return records


def plot_roundtrip_all_files(silesia_records, canterbury_records, output_path: str):
    """
    Plot roundtrip time vs block size for all files, similar to plot_bzip2_bench.py
    but showing time instead of speedup. One line per file.
    """
    # Combine records and add dataset label
    all_records = []
    for r in silesia_records:
        r_copy = r.copy()
        r_copy['dataset'] = 'Silesia'
        all_records.append(r_copy)
    for r in canterbury_records:
        r_copy = r.copy()
        r_copy['dataset'] = 'Canterbury'
        all_records.append(r_copy)
    
    if not all_records:
        print("No records to plot.")
        return
    
    # Get unique files
    files = sorted(set(r['file'] for r in all_records))
    
    plt.figure(figsize=(12, 8))
    
    # Use a colormap to give each file a distinct color
    cmap = plt.get_cmap("tab20")
    n_files = max(len(files), 1)
    marker_cycle = ["o", "s", "D", "^", "v", "<", ">", "P", "X", "*"]
    
    for idx, file_name in enumerate(files):
        file_recs = [r for r in all_records if r['file'] == file_name]
        file_recs.sort(key=lambda x: x['block_size_kb'])
        
        if not file_recs:
            continue
        
        x_vals = [rec['block_size_kb'] for rec in file_recs]
        y_vals = [rec['roundtrip_time_s'] for rec in file_recs]
        
        # Create label with dataset info
        dataset = file_recs[0]['dataset']
        legend_label = f"{file_name} ({dataset})"
        
        color = cmap(idx / max(n_files - 1, 1))
        marker = marker_cycle[idx % len(marker_cycle)]
        plt.plot(x_vals, y_vals, marker=marker, label=legend_label, color=color, linewidth=1.5, markersize=6)
    
    # Configure x-axis ticks to show human-readable block sizes
    all_sizes = sorted(set(r['block_size_kb'] for r in all_records))
    size_to_label = {}
    for r in all_records:
        size_to_label[r['block_size_kb']] = r['block_size_label']
    tick_labels = [size_to_label.get(kb, f"{kb:.2f} KB") for kb in all_sizes]
    
    plt.xticks(all_sizes, tick_labels, rotation=45, ha="right")
    plt.ylabel("Roundtrip Time (seconds)", fontsize=12)
    plt.xlabel("Block Size", fontsize=12)
    plt.title("Roundtrip Time vs Block Size (All Files)", fontsize=14, fontweight='bold')
    plt.grid(True, linestyle="--", alpha=0.5)
    
    # Move legend outside the plotting area
    plt.legend(
        fontsize=8,
        ncol=1,
        loc="upper left",
        bbox_to_anchor=(1.02, 1.0),
        borderaxespad=0.0,
        frameon=True,
    )
    
    plt.tight_layout()
    plt.savefig(output_path, dpi=150, bbox_inches='tight')
    plt.close()
    print(f"Saved plot to {output_path}")


def plot_block_size_vs_roundtrip_time(silesia_records, canterbury_records, output_path: str):
    """
    Create a line chart comparing block size to roundtrip time for both datasets (aggregate).
    """
    # Sort records by block size
    silesia_records.sort(key=lambda x: x['block_size_kb'])
    canterbury_records.sort(key=lambda x: x['block_size_kb'])
    
    # Extract data
    silesia_sizes = [r['block_size_kb'] for r in silesia_records]
    silesia_times = [r['roundtrip_time_s'] for r in silesia_records]
    silesia_labels = [r['block_size_label'] for r in silesia_records]
    
    canterbury_sizes = [r['block_size_kb'] for r in canterbury_records]
    canterbury_times = [r['roundtrip_time_s'] for r in canterbury_records]
    canterbury_labels = [r['block_size_label'] for r in canterbury_records]
    
    # Create the plot
    plt.figure(figsize=(10, 6))
    
    # Plot lines
    plt.plot(silesia_sizes, silesia_times, marker='o', label='Silesia', linewidth=2, markersize=8)
    plt.plot(canterbury_sizes, canterbury_times, marker='s', label='Canterbury', linewidth=2, markersize=8)
    
    # Set labels and title
    plt.xlabel('Block Size', fontsize=12)
    plt.ylabel('Roundtrip Time (seconds)', fontsize=12)
    plt.title('Block Size vs Roundtrip Time (Aggregate)', fontsize=14, fontweight='bold')
    plt.legend(fontsize=11)
    plt.grid(True, linestyle='--', alpha=0.6)
    
    # Set x-axis ticks to show block size labels
    # Use the labels from silesia (they should be the same for both)
    all_sizes = sorted(set(silesia_sizes + canterbury_sizes))
    size_to_label = {}
    for r in silesia_records + canterbury_records:
        size_to_label[r['block_size_kb']] = r['block_size_label']
    
    tick_labels = [size_to_label.get(size, f"{size:.2f} KB") for size in all_sizes]
    plt.xticks(all_sizes, tick_labels, rotation=45, ha='right')
    
    # Use log scale for x-axis if block sizes span multiple orders of magnitude
    if max(all_sizes) / min(all_sizes) > 100:
        plt.xscale('log')
    
    plt.tight_layout()
    plt.savefig(output_path, dpi=150)
    plt.close()
    print(f"Saved plot to {output_path}")


def main():
    parser = argparse.ArgumentParser(
        description="Plot block size vs roundtrip time comparison for silesia and canterbury datasets."
    )
    parser.add_argument(
        "--silesia",
        default="logs/12-12-perf-silesia.log",
        help="Path to the silesia performance log file.",
    )
    parser.add_argument(
        "--canterbury",
        default="logs/12-12-perf-canterbury.log",
        help="Path to the canterbury performance log file.",
    )
    parser.add_argument(
        "--output",
        default="plots_performance_comparison.png",
        help="Output path for the plot.",
    )
    
    args = parser.parse_args()
    
    # Resolve paths relative to project root
    project_root = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
    silesia_path = os.path.join(project_root, args.silesia)
    canterbury_path = os.path.join(project_root, args.canterbury)
    output_path = os.path.join(project_root, args.output)
    
    if not os.path.exists(silesia_path):
        print(f"Error: Silesia log file not found: {silesia_path}")
        return
    
    if not os.path.exists(canterbury_path):
        print(f"Error: Canterbury log file not found: {canterbury_path}")
        return
    
    print(f"Parsing silesia log: {silesia_path}")
    silesia_records = parse_per_file_results(silesia_path)
    print(f"Found {len(silesia_records)} per-file entries for silesia")
    
    print(f"Parsing canterbury log: {canterbury_path}")
    canterbury_records = parse_per_file_results(canterbury_path)
    print(f"Found {len(canterbury_records)} per-file entries for canterbury")
    
    if not silesia_records and not canterbury_records:
        print("Error: No data found in log files.")
        return
    
    # Create output directory if it's a directory path
    output_dir = os.path.dirname(output_path)
    if output_dir and not os.path.exists(output_dir):
        os.makedirs(output_dir, exist_ok=True)
    
    # Plot per-file roundtrip times
    plot_roundtrip_all_files(silesia_records, canterbury_records, output_path)


if __name__ == "__main__":
    main()

