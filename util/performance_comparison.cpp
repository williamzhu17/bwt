/*
 * Performance comparison utilities for BWT benchmarking
 * Handles statistics calculation and result printing
 */

#include "performance_comparison.hpp"
#include "format_utils.hpp"
#include <iostream>
#include <iomanip>
#include <algorithm>
#include <numeric>
#include <cmath>
#include <string>

void TrialResult::calculate_metrics() {
    // Round trip times
    your_roundtrip_time = your_forward_time + your_inverse_time;
    bzip2_roundtrip_time = bzip2_forward_time + bzip2_inverse_time;
    
    // Calculate speedup metrics
    if (your_forward_time > 0) {
        forward_speedup = bzip2_forward_time / your_forward_time;
    }
    if (your_inverse_time > 0) {
        inverse_speedup = bzip2_inverse_time / your_inverse_time;
    }
    if (your_roundtrip_time > 0) {
        roundtrip_speedup = bzip2_roundtrip_time / your_roundtrip_time;
    }
}

double ComparisonResult::calculate_mean(const std::vector<double>& values) {
    if (values.empty()) return 0.0;
    return std::accumulate(values.begin(), values.end(), 0.0) / values.size();
}

double ComparisonResult::calculate_stddev(const std::vector<double>& values, double mean) {
    if (values.size() <= 1) return 0.0;
    double sum_squared_diff = 0.0;
    for (double val : values) {
        double diff = val - mean;
        sum_squared_diff += diff * diff;
    }
    return std::sqrt(sum_squared_diff / (values.size() - 1));
}

void ComparisonResult::calculate_forward_statistics() {
    std::vector<double> your_times, bzip2_times, speedups;
    
    for (const auto& trial : trials) {
        your_times.push_back(trial.your_forward_time);
        bzip2_times.push_back(trial.bzip2_forward_time);
        if (trial.forward_speedup > 0) {
            speedups.push_back(trial.forward_speedup);
        }
    }
    
    your_forward_time_mean = calculate_mean(your_times);
    your_forward_time_stddev = calculate_stddev(your_times, your_forward_time_mean);
    your_forward_time_min = *std::min_element(your_times.begin(), your_times.end());
    your_forward_time_max = *std::max_element(your_times.begin(), your_times.end());
    
    bzip2_forward_time_mean = calculate_mean(bzip2_times);
    bzip2_forward_time_stddev = calculate_stddev(bzip2_times, bzip2_forward_time_mean);
    bzip2_forward_time_min = *std::min_element(bzip2_times.begin(), bzip2_times.end());
    bzip2_forward_time_max = *std::max_element(bzip2_times.begin(), bzip2_times.end());
    
    if (!speedups.empty()) {
        forward_speedup_mean = calculate_mean(speedups);
        forward_speedup_stddev = calculate_stddev(speedups, forward_speedup_mean);
    }
}

void ComparisonResult::calculate_inverse_statistics() {
    std::vector<double> your_times, bzip2_times, speedups;
    
    for (const auto& trial : trials) {
        your_times.push_back(trial.your_inverse_time);
        bzip2_times.push_back(trial.bzip2_inverse_time);
        if (trial.inverse_speedup > 0) {
            speedups.push_back(trial.inverse_speedup);
        }
    }
    
    your_inverse_time_mean = calculate_mean(your_times);
    your_inverse_time_stddev = calculate_stddev(your_times, your_inverse_time_mean);
    your_inverse_time_min = *std::min_element(your_times.begin(), your_times.end());
    your_inverse_time_max = *std::max_element(your_times.begin(), your_times.end());
    
    bzip2_inverse_time_mean = calculate_mean(bzip2_times);
    bzip2_inverse_time_stddev = calculate_stddev(bzip2_times, bzip2_inverse_time_mean);
    bzip2_inverse_time_min = *std::min_element(bzip2_times.begin(), bzip2_times.end());
    bzip2_inverse_time_max = *std::max_element(bzip2_times.begin(), bzip2_times.end());
    
    if (!speedups.empty()) {
        inverse_speedup_mean = calculate_mean(speedups);
        inverse_speedup_stddev = calculate_stddev(speedups, inverse_speedup_mean);
    }
}

void ComparisonResult::calculate_roundtrip_statistics() {
    std::vector<double> your_times, bzip2_times, speedups;
    
    for (const auto& trial : trials) {
        your_times.push_back(trial.your_roundtrip_time);
        bzip2_times.push_back(trial.bzip2_roundtrip_time);
        if (trial.roundtrip_speedup > 0) {
            speedups.push_back(trial.roundtrip_speedup);
        }
    }
    
    your_roundtrip_time_mean = calculate_mean(your_times);
    your_roundtrip_time_stddev = calculate_stddev(your_times, your_roundtrip_time_mean);
    your_roundtrip_time_min = *std::min_element(your_times.begin(), your_times.end());
    your_roundtrip_time_max = *std::max_element(your_times.begin(), your_times.end());
    
    bzip2_roundtrip_time_mean = calculate_mean(bzip2_times);
    bzip2_roundtrip_time_stddev = calculate_stddev(bzip2_times, bzip2_roundtrip_time_mean);
    bzip2_roundtrip_time_min = *std::min_element(bzip2_times.begin(), bzip2_times.end());
    bzip2_roundtrip_time_max = *std::max_element(bzip2_times.begin(), bzip2_times.end());
    
    if (!speedups.empty()) {
        roundtrip_speedup_mean = calculate_mean(speedups);
        roundtrip_speedup_stddev = calculate_stddev(speedups, roundtrip_speedup_mean);
    }
}

void ComparisonResult::calculate_statistics() {
    if (trials.empty()) return;
    
    calculate_forward_statistics();
    calculate_inverse_statistics();
    calculate_roundtrip_statistics();
    
    // Output sizes (should be same across trials, use first)
    your_forward_output_size = trials[0].your_forward_output_size;
    bzip2_forward_output_size = trials[0].bzip2_forward_output_size;
}

void Timer::start() {
    start_time = std::chrono::high_resolution_clock::now();
    is_running = true;
}

double Timer::stop() {
    if (!is_running) return 0.0;
    
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);
    is_running = false;
    return duration.count() / 1000.0;  // Convert to milliseconds
}

void ComparisonPrinter::print_timing_stats(const std::string& label, double mean, double stddev,
                                           double min, double max, size_t num_trials) {
    std::cout << "  " << label << ":" << std::endl;
    std::cout << "    Time:      " << std::fixed << std::setprecision(3) << mean << " ms";
    if (num_trials > 1) {
        std::cout << " ± " << std::fixed << std::setprecision(3) << stddev << " ms";
    }
    std::cout << std::endl;
    std::cout << "    Min:       " << std::fixed << std::setprecision(3) << min << " ms" << std::endl;
    std::cout << "    Max:       " << std::fixed << std::setprecision(3) << max << " ms" << std::endl;
}

void ComparisonPrinter::print_comparison_metrics(const std::string& label, double speedup_mean,
                                                  double speedup_stddev, double your_time,
                                                  double bzip2_time, size_t num_trials) {
    if (speedup_mean > 0) {
        std::cout << "  " << label << ":" << std::endl;
        std::cout << "    Speedup:   " << std::fixed << std::setprecision(3) << speedup_mean << "x";
        if (num_trials > 1) {
            std::cout << " ± " << std::fixed << std::setprecision(3) << speedup_stddev << "x";
        }
        if (speedup_mean < 1.0) {
            std::cout << " (bzip2 is " << std::fixed << std::setprecision(1) 
                      << (1.0 / speedup_mean - 1.0) * 100 << "% faster)";
        } else {
            std::cout << " (your BWT is " << std::fixed << std::setprecision(1) 
                      << (speedup_mean - 1.0) * 100 << "% faster)";
        }
        std::cout << std::endl;
        
        double time_diff = your_time - bzip2_time;
        std::cout << "    Time Diff: " << std::fixed << std::setprecision(3) << time_diff << " ms";
        if (time_diff > 0) {
            std::cout << " (bzip2 is faster)";
        } else {
            std::cout << " (your BWT is faster)";
        }
        std::cout << std::endl;
    }
}

void ComparisonPrinter::print_header(const ComparisonResult& result) {
    std::cout << "\n" << std::string(80, '=') << std::endl;
    std::cout << "Test: " << result.test_name << std::endl;
    std::cout << "Block Size: " << format_size(result.block_size) << std::endl;
    std::cout << "File Size: " << format_size(result.file_size) << std::endl;
    std::cout << "Trials: " << result.num_trials << " (successful: " << result.trials.size() << ")" << std::endl;
    std::cout << std::string(80, '-') << std::endl;
    
    if (result.trials.empty()) {
        std::cout << "ERROR: No successful trials!" << std::endl;
        std::cout << std::string(80, '=') << std::endl;
    }
}

void ComparisonPrinter::print_forward_section(const ComparisonResult& result) {
    std::cout << "\nFORWARD BWT:" << std::endl;
    std::cout << "Your BWT:" << std::endl;
    print_timing_stats("Forward", result.your_forward_time_mean, result.your_forward_time_stddev,
                      result.your_forward_time_min, result.your_forward_time_max, result.trials.size());
    std::cout << "  Output:    " << format_size(result.your_forward_output_size) << std::endl;
    
    std::cout << "\nbzip2 BWT:" << std::endl;
    print_timing_stats("Forward", result.bzip2_forward_time_mean, result.bzip2_forward_time_stddev,
                      result.bzip2_forward_time_min, result.bzip2_forward_time_max, result.trials.size());
    std::cout << "  Output:    " << format_size(result.bzip2_forward_output_size) << std::endl;
    
    print_comparison_metrics("Comparison", result.forward_speedup_mean, result.forward_speedup_stddev,
                            result.your_forward_time_mean, result.bzip2_forward_time_mean, result.trials.size());
}

void ComparisonPrinter::print_inverse_section(const ComparisonResult& result) {
    std::cout << "\nINVERSE BWT:" << std::endl;
    std::cout << "Your BWT:" << std::endl;
    print_timing_stats("Inverse", result.your_inverse_time_mean, result.your_inverse_time_stddev,
                      result.your_inverse_time_min, result.your_inverse_time_max, result.trials.size());
    
    std::cout << "\nbzip2 BWT:" << std::endl;
    print_timing_stats("Inverse", result.bzip2_inverse_time_mean, result.bzip2_inverse_time_stddev,
                      result.bzip2_inverse_time_min, result.bzip2_inverse_time_max, result.trials.size());
    
    print_comparison_metrics("Comparison", result.inverse_speedup_mean, result.inverse_speedup_stddev,
                            result.your_inverse_time_mean, result.bzip2_inverse_time_mean, result.trials.size());
}

void ComparisonPrinter::print_roundtrip_section(const ComparisonResult& result) {
    std::cout << "\nROUND TRIP (Forward + Inverse):" << std::endl;
    std::cout << "Your BWT:" << std::endl;
    print_timing_stats("Round Trip", result.your_roundtrip_time_mean, result.your_roundtrip_time_stddev,
                      result.your_roundtrip_time_min, result.your_roundtrip_time_max, result.trials.size());
    
    std::cout << "\nbzip2 BWT:" << std::endl;
    print_timing_stats("Round Trip", result.bzip2_roundtrip_time_mean, result.bzip2_roundtrip_time_stddev,
                      result.bzip2_roundtrip_time_min, result.bzip2_roundtrip_time_max, result.trials.size());
    
    print_comparison_metrics("Comparison", result.roundtrip_speedup_mean, result.roundtrip_speedup_stddev,
                            result.your_roundtrip_time_mean, result.bzip2_roundtrip_time_mean, result.trials.size());
}

void ComparisonPrinter::print_throughput(const ComparisonResult& result) {
    if (result.roundtrip_speedup_mean > 0) {
        double your_throughput = (result.file_size / (1024.0 * 1024.0)) / (result.your_roundtrip_time_mean / 1000.0);
        double bzip2_throughput = (result.file_size / (1024.0 * 1024.0)) / (result.bzip2_roundtrip_time_mean / 1000.0);
        
        std::cout << "  Throughput:" << std::endl;
        std::cout << "    Your BWT:  " << std::fixed << std::setprecision(2) 
                  << your_throughput << " MB/s" << std::endl;
        std::cout << "    bzip2 BWT: " << std::fixed << std::setprecision(2) 
                  << bzip2_throughput << " MB/s" << std::endl;
    }
}

void ComparisonPrinter::print_comparison(const ComparisonResult& result) {
    print_header(result);
    
    if (result.trials.empty()) {
        return;
    }
    
    print_forward_section(result);
    print_inverse_section(result);
    print_roundtrip_section(result);
    print_throughput(result);
    
    std::cout << std::string(80, '=') << std::endl;
    
    // Output summary lines for aggregation
    if (result.forward_speedup_mean > 0) {
        print_summary_line(result, "forward", result.your_forward_time_mean,
                          result.bzip2_forward_time_mean, result.forward_speedup_mean);
    }
    
    if (result.inverse_speedup_mean > 0) {
        print_summary_line(result, "inverse", result.your_inverse_time_mean,
                          result.bzip2_inverse_time_mean, result.inverse_speedup_mean);
    }
    
    if (result.roundtrip_speedup_mean > 0) {
        print_summary_line(result, "roundtrip", result.your_roundtrip_time_mean,
                          result.bzip2_roundtrip_time_mean, result.roundtrip_speedup_mean);
    }
}

void ComparisonPrinter::print_summary_line(const ComparisonResult& result, const std::string& phase,
                                           double your_time, double bzip2_time, double speedup) {
    // Format: SUMMARY|test_name|phase|your_time_mean|bzip2_time_mean|speedup|winner|faster_by_pct
    std::string winner = (speedup < 1.0) ? "bzip2" : "your_bwt";
    double speedup_pct = (speedup < 1.0) 
        ? (1.0 / speedup - 1.0) * 100.0
        : (speedup - 1.0) * 100.0;
    
    std::cout << "SUMMARY|" << result.test_name << "|" << phase << "|" 
              << std::fixed << std::setprecision(3) << your_time << "|"
              << std::fixed << std::setprecision(3) << bzip2_time << "|"
              << std::fixed << std::setprecision(3) << speedup << "|"
              << winner << "|"
              << std::fixed << std::setprecision(1) << speedup_pct << std::endl;
}

