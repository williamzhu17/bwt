import re
import sys
import os
import argparse
from collections import defaultdict
import matplotlib.pyplot as plt
import numpy as np

class BenchmarkParser:
    def __init__(self):
        self.data = []
        
    def parse_time(self, time_str):
        # Parse "29.8284 ms" or "2.6236 s" or "803.2586 μs" to ms
        parts = time_str.strip().split()
        val = float(parts[0])
        unit = parts[1]
        if unit == 's':
            return val * 1000
        elif unit == 'ms':
            return val
        elif unit == 'μs' or unit == 'us':
            return val / 1000
        return val

    def parse_size(self, size_str):
        # Parse "512 B", "1.00 KB", "16.00 KB" to KB
        parts = size_str.strip().split()
        val = float(parts[0])
        unit = parts[1]
        if unit == 'B':
            return val / 1024
        elif unit == 'KB':
            return val
        elif unit == 'MB':
            return val * 1024
        return val

    def parse_file(self, filepath, language):
        if not os.path.exists(filepath):
            print(f"Warning: File {filepath} not found.")
            return

        current_test = {}
        with open(filepath, 'r', encoding='utf-8') as f:
            lines = f.readlines()
            
        i = 0
        while i < len(lines):
            line = lines[i].strip()
            
            if line.startswith("Test:"):
                # Start new test object
                if current_test: 
                    # In case previous wasn't added (though we add at Throughput usually)
                    pass 
                current_test = {'language': language}
                current_test['filename'] = line.split(":")[1].strip()
            
            elif line.startswith("Block Size:"):
                current_test['block_size_kb'] = self.parse_size(line.split(":")[1].strip())
                # Keep original string for labels if needed, or format consistently
                bs_val = self.parse_size(line.split(":")[1].strip())
                if bs_val < 1:
                    current_test['block_size_label'] = f"{int(bs_val*1024)} B"
                else:
                    current_test['block_size_label'] = f"{bs_val:.2f} KB"
            
            elif line.startswith("Compression Ratio:"):
                 current_test['compression_ratio'] = float(line.split(":")[1].strip())

            elif line.startswith("Forward BWT:"):
                # Look ahead for Mean
                j = i + 1
                while j < len(lines) and j < i + 5:
                    subline = lines[j].strip()
                    if subline.startswith("Mean:"):
                        time_str = subline.split("Mean:")[1].split("±")[0].strip()
                        current_test['forward_time_ms'] = self.parse_time(time_str)
                        break
                    j += 1
            
            elif line.startswith("Inverse BWT:"):
                j = i + 1
                while j < len(lines) and j < i + 5:
                    subline = lines[j].strip()
                    if subline.startswith("Mean:"):
                        time_str = subline.split("Mean:")[1].split("±")[0].strip()
                        current_test['inverse_time_ms'] = self.parse_time(time_str)
                        break
                    j += 1

            elif line.startswith("Total Roundtrip:"):
                j = i + 1
                while j < len(lines) and j < i + 5:
                    subline = lines[j].strip()
                    if subline.startswith("Mean:"):
                        time_str = subline.split("Mean:")[1].split("±")[0].strip()
                        current_test['roundtrip_time_ms'] = self.parse_time(time_str)
                        break
                    j += 1

            elif line.startswith("Throughput:"):
                val_str = line.split(":")[1].strip().split()[0]
                current_test['throughput_mb_s'] = float(val_str)
                
                # Verify we have necessary fields before adding
                if 'filename' in current_test and 'block_size_kb' in current_test:
                    self.data.append(current_test)
                current_test = {} # Reset
            
            i += 1

def aggregate_data(data):
    # Dictionary mapping (language, block_size) -> list of values
    agg = defaultdict(lambda: defaultdict(list))
    
    for entry in data:
        lang = entry['language']
        bs = entry['block_size_kb']
        bs_label = entry['block_size_label']
        
        key = (lang, bs, bs_label)
        agg[key]['throughput'].append(entry.get('throughput_mb_s', 0))
        agg[key]['roundtrip'].append(entry.get('roundtrip_time_ms', 0))
        agg[key]['forward'].append(entry.get('forward_time_ms', 0))
        agg[key]['inverse'].append(entry.get('inverse_time_ms', 0))
        agg[key]['compression'].append(entry.get('compression_ratio', 0))
        
    # Calculate averages
    results = []
    for (lang, bs, bs_label), metrics in agg.items():
        res = {
            'language': lang,
            'block_size_kb': bs,
            'block_size_label': bs_label,
            'avg_throughput': np.mean(metrics['throughput']),
            'avg_roundtrip': np.mean(metrics['roundtrip']),
            'avg_forward': np.mean(metrics['forward']),
            'avg_inverse': np.mean(metrics['inverse']),
            'avg_compression': np.mean(metrics['compression'])
        }
        results.append(res)
    
    return sorted(results, key=lambda x: x['block_size_kb'])

def plot_bandwidth_vs_block_size(results, filename):
    # Separate by language
    cpp_data = [r for r in results if r['language'] == 'C++']
    py_data = [r for r in results if r['language'] == 'Python']
    
    block_sizes = sorted(list(set(r['block_size_kb'] for r in results)))
    
    # Map kb to label
    kb_to_label = {}
    for r in results:
        kb_to_label[r['block_size_kb']] = r['block_size_label']
    sorted_labels = [kb_to_label[bs] for bs in block_sizes]

    x = np.arange(len(block_sizes))
    width = 0.35

    fig, ax = plt.subplots(figsize=(10, 6))
    
    if py_data:
        py_vals = [next((item['avg_throughput'] for item in py_data if item['block_size_kb'] == bs), 0) for bs in block_sizes]
        rects2 = ax.bar(x - width/2, py_vals, width, label='Python')
        ax.bar_label(rects2, padding=3, fmt='%.2f')

    if cpp_data:
        cpp_vals = [next((item['avg_throughput'] for item in cpp_data if item['block_size_kb'] == bs), 0) for bs in block_sizes]
        rects1 = ax.bar(x + width/2, cpp_vals, width, label='C++')
        ax.bar_label(rects1, padding=3, fmt='%.2f')

    ax.set_ylabel('Throughput (MB/s)')
    ax.set_xlabel('Block Size')
    ax.set_title('Average Throughput vs Block Size')
    ax.set_xticks(x)
    ax.set_xticklabels(sorted_labels)
    ax.legend()
    ax.grid(True, axis='y', linestyle='--', alpha=0.7)

    plt.tight_layout()
    plt.savefig(filename)
    print(f"Saved {filename}")

def plot_speedup_based_on_throughput(results, filename):
    # Need both languages present for same block sizes
    cpp_dict = {r['block_size_kb']: r for r in results if r['language'] == 'C++'}
    py_dict = {r['block_size_kb']: r for r in results if r['language'] == 'Python'}
    
    common_sizes = sorted(list(set(cpp_dict.keys()) & set(py_dict.keys())))
    
    if common_sizes:
        speedups = []
        labels = []
        for size in common_sizes:
            cpp_th = cpp_dict[size]['avg_throughput']
            py_th = py_dict[size]['avg_throughput']
            
            # Speedup = C++ throughput / Python throughput (since higher throughput is better)
            # OR Speedup = Python Time / C++ Time (which is ~ equivalent to C++ Throughput / Python Throughput)
            if py_th > 0:
                speedup = cpp_th / py_th
                speedups.append(speedup)
                labels.append(cpp_dict[size]['block_size_label'])
        
        plt.figure(figsize=(10, 6))
        bars = plt.bar(labels, speedups, color='green')
        plt.ylabel('Speedup Factor (C++ Throughput / Python Throughput)')
        plt.xlabel('Block Size')
        plt.title('C++ Speedup over Python (Throughput)')
        plt.grid(True, axis='y', linestyle='--', alpha=0.7)
        plt.bar_label(bars, fmt='%.1fx')
        
        plt.tight_layout()
        plt.savefig(filename)
        print(f"Saved {filename}")

def plot_combined_per_file_throughput(data, output_file):
    files = sorted(list(set(d['filename'] for d in data)))
    block_sizes = sorted(list(set(d['block_size_kb'] for d in data)))
    
    # We want to show individual bars for each block size for each file, but grouped by file.
    # So X-axis = Files.
    # For each file, we have 2 * num_block_sizes bars.
    
    # This might get very crowded. 11 files * 4 block sizes * 2 languages = 88 bars.
    # It fits on a wide figure.
    
    n_files = len(files)
    n_blocks = len(block_sizes)
    total_width = 0.8 # Width of the group for one file
    bar_width = total_width / (n_blocks * 2)
    
    fig, ax = plt.subplots(figsize=(20, 8))
    
    # Colors - maybe shades?
    # Python: Oranges
    # C++: Blues
    py_colors = plt.cm.Oranges(np.linspace(0.3, 0.7, n_blocks))
    cpp_colors = plt.cm.Blues(np.linspace(0.3, 0.7, n_blocks))
    
    x = np.arange(n_files)
    
    # We need to position bars.
    # Center of file group is x[i]
    # Range is from x[i] - total_width/2 to x[i] + total_width/2
    
    # Order: Python (BS1, BS2, BS3, BS4) then C++ (BS1, BS2, BS3, BS4)
    # Or Interleaved: (Py BS1, C++ BS1), (Py BS2, C++ BS2)...
    # User asked "Python on left side of bars". Usually implies grouped by category.
    # Let's do Interleaved: Py BS1, C++ BS1, Py BS2, C++ BS2 ...
    
    # Wait, previous request was "Python on left side of bars".
    # So for a given block size, Python is left of C++.
    
    for i, bs in enumerate(block_sizes):
        # Calculate offset for this block size pair
        # We have n_blocks pairs.
        # Pair i is centered at:
        # start = -total_width/2
        # step = total_width / n_blocks
        # pair_center = start + i * step + step/2
        
        step = total_width / n_blocks
        pair_center_offset = -total_width/2 + i * step + step/2
        
        # Py bar is to the left of pair_center
        # C++ bar is to the right of pair_center
        
        py_offset = pair_center_offset - bar_width/2
        cpp_offset = pair_center_offset + bar_width/2
        
        py_vals = []
        cpp_vals = []
        
        for fname in files:
            # Get data for this file and block size
            d_list = [d for d in data if d['filename'] == fname and d['block_size_kb'] == bs]
            
            # Aggregate (average trials)
            agg_res = aggregate_data(d_list)
            
            p_val = next((r['avg_throughput'] for r in agg_res if r['language'] == 'Python'), 0)
            c_val = next((r['avg_throughput'] for r in agg_res if r['language'] == 'C++'), 0)
            
            py_vals.append(p_val)
            cpp_vals.append(c_val)
            
        # Plot bars
        # Use simple labels only for first block size to avoid legend clutter
        label_py = 'Python' if i == 0 else None
        label_cpp = 'C++' if i == 0 else None
        
        # Use consistent color for language, maybe alpha for block size?
        # Or just legend for block sizes?
        # Let's use colors to distinguish block sizes, and maybe hatch/pattern for language?
        # Or colors for language and alpha/shade for block size.
        
        rects1 = ax.bar(x + py_offset, py_vals, bar_width, label=label_py, color=py_colors[i])
        rects2 = ax.bar(x + cpp_offset, cpp_vals, bar_width, label=label_cpp, color=cpp_colors[i])
        
    ax.set_ylabel('Throughput (MB/s)')
    ax.set_title('Throughput per File per Block Size')
    ax.set_xticks(x)
    ax.set_xticklabels(files, rotation=45, ha='right')
    
    # Custom legend
    from matplotlib.patches import Patch
    legend_elements = []
    
    # Add language colors to legend
    legend_elements.append(Patch(facecolor='orange', label='Python'))
    legend_elements.append(Patch(facecolor='blue', label='C++'))
    
    # Add block size shades to legend - this is tricky to visualize cleanly in legend
    # Instead, let's make the colors consistent for block sizes and hatch for language?
    # Or stick to the shades:
    # Darker = larger block size?
    # Let's just list the block sizes in the legend with their corresponding shade for one language (e.g. C++)
    # and say "Shade intensity indicates block size"
    
    # Actually, calculating the specific colors used:
    # py_colors = plt.cm.Oranges(np.linspace(0.3, 0.7, n_blocks))
    # cpp_colors = plt.cm.Blues(np.linspace(0.3, 0.7, n_blocks))
    
    # Let's add legend entries for block sizes
    for i, bs in enumerate(block_sizes):
         # Create a dummy patch with the C++ color
         bs_label = [d['block_size_label'] for d in data if d['block_size_kb'] == bs][0]
         legend_elements.append(Patch(facecolor=cpp_colors[i], label=f"{bs_label} (C++ shade)"))

    ax.legend(handles=legend_elements, loc='upper right')
    ax.grid(True, axis='y', linestyle='--', alpha=0.5)
    
    plt.tight_layout()
    plt.savefig(output_file)
    print(f"Saved {output_file}")

def main():
    parser = argparse.ArgumentParser(description='Plot BWT Performance')
    parser.add_argument('--cpp', required=True, help='Path to C++ log file')
    parser.add_argument('--python', required=True, help='Path to Python log file')
    parser.add_argument('--output', default='plots', help='Output directory for plots')
    
    args = parser.parse_args()
    
    if not os.path.exists(args.output):
        os.makedirs(args.output)
        
    bp = BenchmarkParser()
    
    # Check if files exist before parsing
    if os.path.exists(args.cpp):
        print(f"Parsing C++ log: {args.cpp}")
        bp.parse_file(args.cpp, 'C++')
    else:
        print(f"C++ log file not found: {args.cpp}")

    if os.path.exists(args.python):
        print(f"Parsing Python log: {args.python}")
        bp.parse_file(args.python, 'Python')
    else:
        print(f"Python log file not found: {args.python}")
        
    if not bp.data:
        print("No data found to plot.")
        return

    agg_results = aggregate_data(bp.data)
    
    # 1. Plot Average Bandwidth (Throughput) vs Block Size
    plot_bandwidth_vs_block_size(agg_results, os.path.join(args.output, 'average_bandwidth.png'))
    
    # 2. Plot Speedup based on Throughput
    plot_speedup_based_on_throughput(agg_results, os.path.join(args.output, 'throughput_speedup.png'))

    # 3. Plot combined per file
    plot_combined_per_file_throughput(bp.data, os.path.join(args.output, 'combined_per_file_throughput.png'))

if __name__ == "__main__":
    main()
