import os
import argparse
from collections import defaultdict

import matplotlib.pyplot as plt


def parse_size_to_kb(size_str: str) -> float:
    """
    Parse strings like '64.00 KB', '1.00 MB' into KB.
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


def parse_bzip2_canterbury_log(filepath: str):
    """
    Parse the 12-2-canterbury-bzip2-bench.log file.

    We rely on lines of the form:
      Test: <filename>
      Block Size: <size>
      SUMMARY|<file>|<phase>|<your_ms>|<bzip2_ms>|<speedup>|<winner>|<pct>
    """
    records = []

    current_file = None
    current_block_size_kb = None
    current_block_size_label = None
    current_file_size_kb = None
    current_file_size_label = None

    with open(filepath, "r", encoding="utf-8") as f:
        for raw_line in f:
            line = raw_line.strip()

            if line.startswith("Test:"):
                # Example: "Test: alice29.txt"
                current_file = line.split(":", 1)[1].strip()

            elif line.startswith("File Size:"):
                # Example: "File Size: 148.52 KB"
                size_str = line.split(":", 1)[1].strip()
                current_file_size_kb = parse_size_to_kb(size_str)
                current_file_size_label = size_str

            elif line.startswith("Block Size:"):
                # Example: "Block Size: 64.00 KB"
                size_str = line.split(":", 1)[1].strip()
                kb = parse_size_to_kb(size_str)
                current_block_size_kb = kb
                # Keep a nice label (match the original size string)
                current_block_size_label = size_str

            elif line.startswith("SUMMARY|"):
                # Example:
                # SUMMARY|alice29.txt|forward|11.505|5.828|0.508|bzip2|96.9
                parts = line.split("|")
                if len(parts) != 8:
                    continue

                _, file_name, phase, your_ms, bzip2_ms, speedup, winner, pct = parts

                try:
                    record = {
                        "file": file_name,
                        "phase": phase,  # forward / inverse / roundtrip
                        "your_ms": float(your_ms),
                        "bzip2_ms": float(bzip2_ms),
                        "speedup": float(speedup),
                        "winner": winner,
                        "pct_faster": float(pct),
                        "block_size_kb": current_block_size_kb,
                        "block_size_label": current_block_size_label,
                        "file_size_kb": current_file_size_kb,
                        "file_size_label": current_file_size_label,
                    }
                except ValueError:
                    # Skip malformed numeric entries
                    continue

                records.append(record)

    return records


def plot_per_file(records, output_dir: str):
    """
    For each test file, create a figure with three subplots (forward, inverse,
    roundtrip) comparing your BWT vs bzip2 across block sizes.
    """
    if not records:
        print("No records to plot.")
        return

    files = sorted({r["file"] for r in records})
    phases = ["forward", "inverse", "roundtrip"]

    for file_name in files:
        fig, axes = plt.subplots(1, 3, figsize=(15, 4), sharey=False)
        fig.suptitle(f"BWT Timing vs Block Size: {file_name}", fontsize=14)

        file_records = [r for r in records if r["file"] == file_name]

        for idx, phase in enumerate(phases):
            ax = axes[idx]
            phase_records = [
                r
                for r in file_records
                if r["phase"] == phase
                and r["block_size_kb"] is not None
                and r["block_size_kb"] <= 512.0  # exclude 1 MB
            ]
            # Some files/phases may be missing (e.g., failed trials or excluded 1 MB)
            if not phase_records:
                ax.set_title(f"{phase.capitalize()} (no data)")
                ax.axis("off")
                continue

            # Sort by block size
            phase_records.sort(key=lambda x: x["block_size_kb"])

            x_labels = [r["block_size_label"] for r in phase_records]
            x_pos = range(len(phase_records))
            your_times = [r["your_ms"] for r in phase_records]
            bzip2_times = [r["bzip2_ms"] for r in phase_records]

            ax.plot(x_pos, your_times, marker="o", label="Your BWT")
            ax.plot(x_pos, bzip2_times, marker="s", label="bzip2 BWT")
            ax.set_title(phase.capitalize())
            ax.set_xlabel("Block Size")
            if idx == 0:
                ax.set_ylabel("Time (ms)")
            ax.set_xticks(list(x_pos))
            ax.set_xticklabels(x_labels, rotation=45, ha="right", fontsize=8)
            ax.grid(True, linestyle="--", alpha=0.4)

        # Put legend outside to avoid overlap
        handles, labels = axes[0].get_legend_handles_labels()
        fig.legend(handles, labels, loc="upper right")
        fig.tight_layout(rect=[0, 0, 0.98, 0.92])

        # Sanitize filename for output
        safe_name = file_name.replace("/", "_")
        out_path = os.path.join(output_dir, f"{safe_name}_bwt_timing.png")
        fig.savefig(out_path, dpi=150)
        plt.close(fig)
        print(f"Saved {out_path}")


def plot_speedup_histogram(records, output_dir: str):
    """
    Optional aggregate: histogram of speedup factors (your / bzip2) by phase.
    """
    if not records:
        return

    phases = ["forward", "inverse", "roundtrip"]
    plt.figure(figsize=(10, 4))

    for idx, phase in enumerate(phases, start=1):
        plt.subplot(1, 3, idx)
        # Exclude 1 MB entries (block_size_kb > 512)
        phase_records = [
            r
            for r in records
            if r["phase"] == phase
            and r["block_size_kb"] is not None
            and r["block_size_kb"] <= 512.0
        ]
        if not phase_records:
            plt.title(f"{phase.capitalize()} (no data)")
            continue
        speedups = [r["speedup"] for r in phase_records]
        plt.hist(speedups, bins=10, alpha=0.7, edgecolor="black")
        plt.title(phase.capitalize())
        plt.xlabel("Speedup (your / bzip2)")
        if idx == 1:
            plt.ylabel("Count")

    plt.tight_layout()
    out_path = os.path.join(output_dir, "speedup_histograms.png")
    plt.savefig(out_path, dpi=150)
    plt.close()
    print(f"Saved {out_path}")


def plot_roundtrip_all_files(records, output_dir: str):
    """
    Combine all test files into a single plot, using only the roundtrip phase.
    One line per file:
      x = block size (KB),
      y = speedup = bzip2_time / your_time (so values > 1 mean your BWT is faster).
    """
    # Use only roundtrip results for block sizes up to and including 512 KB
    roundtrip_records = [
        r
        for r in records
        if r["phase"] == "roundtrip"
        and r["block_size_kb"] is not None
        and r["block_size_kb"] <= 512.0
    ]
    if not roundtrip_records:
        print("No roundtrip records found; skipping combined roundtrip plot.")
        return

    files = sorted({r["file"] for r in roundtrip_records})

    plt.figure(figsize=(10, 6))

    # Use a colormap to give each file a distinct color so lines don't repeat visually
    cmap = plt.get_cmap("tab20")
    n_files = max(len(files), 1)
    marker_cycle = ["o", "s", "D", "^", "v", "<", ">", "P", "X", "*"]

    for idx, file_name in enumerate(files):
        file_recs = [r for r in roundtrip_records if r["file"] == file_name]
        file_recs.sort(key=lambda x: x["block_size_kb"])
        if not file_recs:
            continue

        x_vals = [rec["block_size_kb"] for rec in file_recs]
        # Compute percent speed difference from times so interpretation is clear:
        #   rel_pct = (bzip2_time / your_time - 1) * 100
        #   > 0  -> your implementation is faster (percent faster than bzip2)
        #   < 0  -> bzip2 is faster
        rel_pcts = []
        for rec in file_recs:
            your_t = rec["your_ms"]
            bzip2_t = rec["bzip2_ms"]
            if your_t > 0:
                rel_pcts.append((bzip2_t / your_t - 1.0) * 100.0)
            else:
                rel_pcts.append(0.0)

        # Include file size in legend label for audience clarity
        size_label = next(
            (r.get("file_size_label") for r in file_recs if r.get("file_size_label")),
            None,
        )
        legend_label = f"{file_name} ({size_label})" if size_label else file_name

        color = cmap(idx / max(n_files - 1, 1))
        marker = marker_cycle[idx % len(marker_cycle)]
        plt.plot(x_vals, rel_pcts, marker=marker, label=legend_label, color=color)

    # Configure x-axis ticks to show human-readable block sizes
    unique_sizes = sorted({rec["block_size_kb"] for rec in roundtrip_records})
    size_to_label = {}
    for rec in roundtrip_records:
        size_to_label[rec["block_size_kb"]] = rec["block_size_label"]
    tick_labels = [size_to_label[kb] for kb in unique_sizes]

    plt.xticks(unique_sizes, tick_labels, rotation=45, ha="right")
    ax = plt.gca()
    # 0% line = equal speed
    ax.axhline(0.0, color="gray", linestyle="--", linewidth=1, alpha=0.7)
    plt.ylabel("Roundtrip speedup (%)")
    plt.xlabel("Block Size")
    plt.title("Roundtrip Speedup vs Block Size (Single Thread)")
    plt.grid(True, linestyle="--", alpha=0.5)
    # Move legend outside the plotting area so it doesn't occlude lines
    plt.legend(
        fontsize=8,
        ncol=1,
        loc="upper left",
        bbox_to_anchor=(1.02, 1.0),
        borderaxespad=0.0,
        frameon=False,
    )

    plt.tight_layout()
    out_path = os.path.join(output_dir, "roundtrip_speedup_all_files.png")
    plt.savefig(out_path, dpi=150)
    plt.close()
    print(f"Saved {out_path}")


def plot_roundtrip_all_files_log_ratio(records, output_dir: str):
    """
    Alternate view: plot roundtrip speedup as a ratio (bzip2 / our time) on a log
    y-axis so clustered points near 0% are easier to see. Values < 1 mean our BWT
    is faster; > 1 means bzip2 is faster.
    """
    roundtrip_records = [
        r
        for r in records
        if r["phase"] == "roundtrip"
        and r["block_size_kb"] is not None
        and r["block_size_kb"] <= 512.0
    ]
    if not roundtrip_records:
        print("No roundtrip records found; skipping log-ratio plot.")
        return

    files = sorted({r["file"] for r in roundtrip_records})

    plt.figure(figsize=(10, 6))

    cmap = plt.get_cmap("tab20")
    n_files = max(len(files), 1)
    marker_cycle = ["o", "s", "D", "^", "v", "<", ">", "P", "X", "*"]

    for idx, file_name in enumerate(files):
        file_recs = [r for r in roundtrip_records if r["file"] == file_name]
        file_recs.sort(key=lambda x: x["block_size_kb"])
        if not file_recs:
            continue

        x_vals = [rec["block_size_kb"] for rec in file_recs]
        ratios = []
        for rec in file_recs:
            your_t = rec["your_ms"]
            bzip2_t = rec["bzip2_ms"]
            if your_t > 0:
                ratios.append(bzip2_t / your_t)
            else:
                ratios.append(1.0)

        size_label = next(
            (r.get("file_size_label") for r in file_recs if r.get("file_size_label")),
            None,
        )
        legend_label = f"{file_name} ({size_label})" if size_label else file_name

        color = cmap(idx / max(n_files - 1, 1))
        marker = marker_cycle[idx % len(marker_cycle)]
        plt.plot(x_vals, ratios, marker=marker, label=legend_label, color=color)

    unique_sizes = sorted({rec["block_size_kb"] for rec in roundtrip_records})
    size_to_label = {rec["block_size_kb"]: rec["block_size_label"] for rec in roundtrip_records}
    tick_labels = [size_to_label[kb] for kb in unique_sizes]

    plt.xticks(unique_sizes, tick_labels, rotation=45, ha="right")
    ax = plt.gca()
    ax.axhline(1.0, color="gray", linestyle="--", linewidth=1, alpha=0.7)
    ax.set_yscale("log")
    plt.ylabel("bzip2 time / our time (log scale)\n(< 1 = our BWT faster)")
    plt.xlabel("Block Size")
    plt.title("Roundtrip Speedup (log ratio, all files)")
    plt.grid(True, which="both", axis="y", linestyle="--", alpha=0.5)
    plt.legend(
        fontsize=8,
        ncol=1,
        loc="upper left",
        bbox_to_anchor=(1.02, 1.0),
        borderaxespad=0.0,
        frameon=False,
    )

    plt.tight_layout()
    out_path = os.path.join(output_dir, "roundtrip_speedup_log_all_files.png")
    plt.savefig(out_path, dpi=150)
    plt.close()
    print(f"Saved {out_path}")


def plot_forward_vs_inverse_speedup(records, output_dir: str):
    """
    Create a simple figure showing how, on average, forward is slower than bzip2
    while inverse is faster.

    We compute the average relative percent speed difference over all files and
    block sizes (<= 512 KB) for forward and inverse separately:
        rel_pct = (bzip2_time / your_time - 1) * 100
      > 0  => our BWT faster
      < 0  => bzip2 faster
    """
    # Filter out 1MB and any missing sizes
    filtered = [
        r
        for r in records
        if r["block_size_kb"] is not None
        and r["block_size_kb"] <= 512.0
        and r["phase"] in ("forward", "inverse")
    ]
    if not filtered:
        print("No forward/inverse records available for aggregate speedup plot.")
        return

    phase_to_rel = {"forward": [], "inverse": []}

    for rec in filtered:
        your_t = rec["your_ms"]
        bzip2_t = rec["bzip2_ms"]
        if your_t <= 0:
            continue
        rel_pct = (bzip2_t / your_t - 1.0) * 100.0
        phase_to_rel[rec["phase"]].append(rel_pct)

    if not phase_to_rel["forward"] or not phase_to_rel["inverse"]:
        print("Insufficient data for forward/inverse aggregate speedup plot.")
        return

    import numpy as np

    phases = ["forward", "inverse"]
    avgs = [np.mean(phase_to_rel[p]) for p in phases]
    errs = [np.std(phase_to_rel[p]) for p in phases]

    plt.figure(figsize=(6, 4))
    x = range(len(phases))
    bars = plt.bar(x, avgs, yerr=errs, capsize=5, color=["tab:orange", "tab:blue"])

    plt.axhline(0.0, color="gray", linestyle="--", linewidth=1, alpha=0.7)
    plt.xticks(x, ["Forward", "Inverse"])
    plt.ylabel("Speedup vs bzip2 (%)")
    plt.title("Forward vs Inverse Average Speedup (Single Thread)")
    plt.grid(True, axis="y", linestyle="--", alpha=0.4)
    # Manually place labels near the top of each bar, offset to the left to avoid
    # sitting on the center error bar line.
    ax = plt.gca()
    for bar, val in zip(bars, avgs):
        x = bar.get_x()
        width = bar.get_width()
        y = bar.get_height()
        # Position slightly inside the bar top, left side
        ax.text(
            x + 0.15 * width,
            y,
            f"{val:.1f}%",
            ha="left",
            va="bottom" if y >= 0 else "top",
            fontsize=9,
        )

    # Ensure error bars aren't clipped by giving some vertical padding
    ymin = min(a - e for a, e in zip(avgs, errs))
    ymax = max(a + e for a, e in zip(avgs, errs))
    yrange = ymax - ymin if ymax > ymin else 1.0
    padding = 0.1 * yrange
    plt.ylim(ymin - padding, ymax + padding)

    plt.tight_layout()
    out_path = os.path.join(output_dir, "forward_vs_inverse_speedup.png")
    plt.savefig(out_path, dpi=150)
    plt.close()
    print(f"Saved {out_path}")

def main():
    parser = argparse.ArgumentParser(
        description="Parse 12-2-canterbury-bzip2-bench.log and plot BWT performance graphs."
    )
    parser.add_argument(
        "--log",
        default="logs/12-2-canterbury-bzip2-bench.log",
        help="Path to the canterbury bzip2 benchmark log file.",
    )
    parser.add_argument(
        "--output",
        default="plots_canterbury_bzip2",
        help="Output directory for generated plots.",
    )

    args = parser.parse_args()

    log_path = os.path.abspath(args.log)
    output_dir = os.path.abspath(args.output)

    if not os.path.exists(log_path):
        print(f"Log file not found: {log_path}")
        return

    os.makedirs(output_dir, exist_ok=True)

    print(f"Parsing log: {log_path}")
    records = parse_bzip2_canterbury_log(log_path)
    if not records:
        print("No SUMMARY records found in log; nothing to plot.")
        return

    print(f"Parsed {len(records)} summary records.")
    plot_per_file(records, output_dir)
    plot_speedup_histogram(records, output_dir)
    plot_roundtrip_all_files(records, output_dir)
    plot_forward_vs_inverse_speedup(records, output_dir)
    plot_roundtrip_all_files_log_ratio(records, output_dir)


if __name__ == "__main__":
    main()


