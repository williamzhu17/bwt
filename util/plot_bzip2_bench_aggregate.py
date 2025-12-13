import os
import argparse

from plot_bzip2_bench import (
    parse_bzip2_canterbury_log,
    plot_roundtrip_all_files,
    plot_roundtrip_all_files_log_ratio,
    plot_forward_vs_inverse_speedup,
)


def main():
    parser = argparse.ArgumentParser(
        description=(
            "Aggregate BWT benchmark results from both Canterbury and Silesia logs "
            "and plot a combined roundtrip speedup figure."
        )
    )
    parser.add_argument(
        "--canterbury-log",
        default="logs/12-11-canterbury-bzip2-bench-single-thread.log",
        help="Path to the Canterbury benchmark log file.",
    )
    parser.add_argument(
        "--silesia-log",
        default="logs/12-11-silesia-bzip2-bench-single-thread.log",
        help="Path to the Silesia benchmark log file.",
    )
    parser.add_argument(
        "--output",
        default="plots_both_bzip2-single-thread",
        help="Output directory for the combined plot.",
    )

    args = parser.parse_args()

    canterbury_path = os.path.abspath(args.canterbury_log)
    silesia_path = os.path.abspath(args.silesia_log)
    output_dir = os.path.abspath(args.output)

    records = []

    if os.path.exists(canterbury_path):
        print(f"Parsing Canterbury log: {canterbury_path}")
        records.extend(parse_bzip2_canterbury_log(canterbury_path))
    else:
        print(f"Canterbury log file not found: {canterbury_path}")

    if os.path.exists(silesia_path):
        print(f"Parsing Silesia log: {silesia_path}")
        records.extend(parse_bzip2_canterbury_log(silesia_path))
    else:
        print(f"Silesia log file not found: {silesia_path}")

    if not records:
        print("No SUMMARY records found in either log; nothing to plot.")
        return

    os.makedirs(output_dir, exist_ok=True)

    # Reuse existing roundtrip aggregation/plotting, which:
    #   - uses only roundtrip phase
    #   - restricts to block sizes <= 512 KB
    #   - labels each line with file name and size
    plot_roundtrip_all_files(records, output_dir)
    # Log-scale variant to reduce clumping near parity
    plot_roundtrip_all_files_log_ratio(records, output_dir)
    # And aggregate forward vs inverse speedup across both corpora
    plot_forward_vs_inverse_speedup(records, output_dir)


if __name__ == "__main__":
    main()


