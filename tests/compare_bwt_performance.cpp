/*
 * Performance comparison between your BWT and bzip2's BWT
 * 
 * This program runs both implementations and compares their performance
 */

#include <iostream>
#include <string>
#include <vector>
#include <cstdio>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <cstring>
#include <algorithm>
#include <numeric>
#include <cmath>
#include "../src/file_processor.hpp"
#include "../src/bwt.hpp"
#include "../util/file_utils.hpp"
#include "../util/format_utils.hpp"

// Configuration
const int DEFAULT_NUM_TRIALS = 5;

struct TrialResult {
    double your_time = 0.0;
    double bzip2_time = 0.0;
    size_t your_output_size = 0;
    size_t bzip2_output_size = 0;
    double speedup = 0.0;
    double time_diff = 0.0;
};

struct ComparisonResult {
    std::string test_name;
    size_t block_size;
    size_t file_size;
    int num_trials = 0;
    
    // Trial results
    std::vector<TrialResult> trials;
    
    // Statistics (calculated from trials)
    double your_time_mean = 0.0;
    double your_time_stddev = 0.0;
    double your_time_min = 0.0;
    double your_time_max = 0.0;
    
    double bzip2_time_mean = 0.0;
    double bzip2_time_stddev = 0.0;
    double bzip2_time_min = 0.0;
    double bzip2_time_max = 0.0;
    
    double speedup_mean = 0.0;
    double speedup_stddev = 0.0;
    
    size_t your_output_size = 0;
    size_t bzip2_output_size = 0;
    
    void calculate_statistics() {
        if (trials.empty()) return;
        
        // Extract time vectors
        std::vector<double> your_times, bzip2_times, speedups;
        for (const auto& trial : trials) {
            your_times.push_back(trial.your_time);
            bzip2_times.push_back(trial.bzip2_time);
            if (trial.speedup > 0) {
                speedups.push_back(trial.speedup);
            }
        }
        
        // Calculate statistics
        your_time_mean = calculate_mean(your_times);
        your_time_stddev = calculate_stddev(your_times, your_time_mean);
        your_time_min = *std::min_element(your_times.begin(), your_times.end());
        your_time_max = *std::max_element(your_times.begin(), your_times.end());
        
        bzip2_time_mean = calculate_mean(bzip2_times);
        bzip2_time_stddev = calculate_stddev(bzip2_times, bzip2_time_mean);
        bzip2_time_min = *std::min_element(bzip2_times.begin(), bzip2_times.end());
        bzip2_time_max = *std::max_element(bzip2_times.begin(), bzip2_times.end());
        
        if (!speedups.empty()) {
            speedup_mean = calculate_mean(speedups);
            speedup_stddev = calculate_stddev(speedups, speedup_mean);
        }
        
        // Output sizes (should be same across trials, use first)
        if (!trials.empty()) {
            your_output_size = trials[0].your_output_size;
            bzip2_output_size = trials[0].bzip2_output_size;
        }
    }
    
private:
    double calculate_mean(const std::vector<double>& values) {
        if (values.empty()) return 0.0;
        return std::accumulate(values.begin(), values.end(), 0.0) / values.size();
    }
    
    double calculate_stddev(const std::vector<double>& values, double mean) {
        if (values.size() <= 1) return 0.0;
        double sum_squared_diff = 0.0;
        for (double val : values) {
            double diff = val - mean;
            sum_squared_diff += diff * diff;
        }
        return std::sqrt(sum_squared_diff / (values.size() - 1));
    }
};

// Run your BWT implementation
bool run_your_bwt(const std::string& input_file, const std::string& output_file, 
                  size_t block_size, double& elapsed_time, size_t& output_size) {
    auto start = std::chrono::high_resolution_clock::now();
    
    int result = bwt_forward_process_file(input_file.c_str(), output_file.c_str(), block_size);
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    elapsed_time = duration.count() / 1000.0;  // Convert to milliseconds
    
    if (result != 0) {
        return false;
    }
    
    output_size = get_file_size(output_file);
    return true;
}

// Run bzip2 BWT extractor (assumes it's compiled as bzip2_bwt_extractor)
bool run_bzip2_bwt(const std::string& input_file, const std::string& output_file,
                   size_t block_size, double& elapsed_time, size_t& output_size) {
    std::string command = "./build/bzip2_bwt_extractor \"" + input_file + "\" \"" + 
                          output_file + "\" " + std::to_string(block_size);
    
    auto start = std::chrono::high_resolution_clock::now();
    
    int result = system(command.c_str());
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    elapsed_time = duration.count() / 1000.0;  // Convert to milliseconds
    
    if (result != 0) {
        return false;
    }
    
    output_size = get_file_size(output_file);
    return true;
}

// Compare implementations on a single file with multiple trials
ComparisonResult compare_implementations(const std::string& input_file, 
                                         const std::string& test_name,
                                         size_t block_size,
                                         int num_trials) {
    ComparisonResult result;
    result.test_name = test_name;
    result.block_size = block_size;
    result.file_size = get_file_size(input_file);
    result.num_trials = num_trials;
    
    std::string your_output = "build/tmp/your_bwt_output.bwt";
    std::string bzip2_output = "build/tmp/bzip2_bwt_output.bwt";
    
    // Run multiple trials
    for (int trial = 0; trial < num_trials; trial++) {
        TrialResult trial_result;
        
        // Clean up previous outputs
        std::remove(your_output.c_str());
        std::remove(bzip2_output.c_str());
        
        // Run your BWT
        bool your_success = run_your_bwt(input_file, your_output, block_size, 
                                         trial_result.your_time, trial_result.your_output_size);
        
        // Run bzip2 BWT
        bool bzip2_success = run_bzip2_bwt(input_file, bzip2_output, block_size,
                                           trial_result.bzip2_time, trial_result.bzip2_output_size);
        
        if (!your_success) {
            std::cerr << "Warning: Your BWT failed for " << test_name << " (trial " << (trial + 1) << ")" << std::endl;
            continue;
        }
        
        if (!bzip2_success) {
            std::cerr << "Warning: bzip2 BWT failed for " << test_name << " (trial " << (trial + 1) << ")" << std::endl;
            continue;
        }
        
        // Calculate comparison metrics
        if (trial_result.your_time > 0) {
            trial_result.speedup = trial_result.bzip2_time / trial_result.your_time;
        }
        trial_result.time_diff = trial_result.your_time - trial_result.bzip2_time;
        
        result.trials.push_back(trial_result);
    }
    
    // Calculate statistics
    result.calculate_statistics();
    
    // Clean up
    std::remove(your_output.c_str());
    std::remove(bzip2_output.c_str());
    
    return result;
}

// Print comparison results with statistics
void print_comparison(const ComparisonResult& result) {
    std::cout << "\n" << std::string(80, '=') << std::endl;
    std::cout << "Test: " << result.test_name << std::endl;
    std::cout << "Block Size: " << format_size(result.block_size) << std::endl;
    std::cout << "File Size: " << format_size(result.file_size) << std::endl;
    std::cout << "Trials: " << result.num_trials << " (successful: " << result.trials.size() << ")" << std::endl;
    std::cout << std::string(80, '-') << std::endl;
    
    if (result.trials.empty()) {
        std::cout << "ERROR: No successful trials!" << std::endl;
        std::cout << std::string(80, '=') << std::endl;
        return;
    }
    
    std::cout << "Your BWT:" << std::endl;
    std::cout << "  Time:      " << std::fixed << std::setprecision(3) 
              << result.your_time_mean << " ms";
    if (result.trials.size() > 1) {
        std::cout << " ± " << std::fixed << std::setprecision(3) << result.your_time_stddev << " ms";
    }
    std::cout << std::endl;
    std::cout << "  Min:       " << std::fixed << std::setprecision(3) 
              << result.your_time_min << " ms" << std::endl;
    std::cout << "  Max:       " << std::fixed << std::setprecision(3) 
              << result.your_time_max << " ms" << std::endl;
    std::cout << "  Output:    " << format_size(result.your_output_size) << std::endl;
    
    std::cout << "\nbzip2 BWT:" << std::endl;
    std::cout << "  Time:      " << std::fixed << std::setprecision(3) 
              << result.bzip2_time_mean << " ms";
    if (result.trials.size() > 1) {
        std::cout << " ± " << std::fixed << std::setprecision(3) << result.bzip2_time_stddev << " ms";
    }
    std::cout << std::endl;
    std::cout << "  Min:       " << std::fixed << std::setprecision(3) 
              << result.bzip2_time_min << " ms" << std::endl;
    std::cout << "  Max:       " << std::fixed << std::setprecision(3) 
              << result.bzip2_time_max << " ms" << std::endl;
    std::cout << "  Output:    " << format_size(result.bzip2_output_size) << std::endl;
    
    if (result.speedup_mean > 0) {
        std::cout << "\nComparison:" << std::endl;
        std::cout << "  Speedup:   " << std::fixed << std::setprecision(3) 
                  << result.speedup_mean << "x";
        if (result.trials.size() > 1) {
            std::cout << " ± " << std::fixed << std::setprecision(3) << result.speedup_stddev << "x";
        }
        if (result.speedup_mean > 1.0) {
            std::cout << " (bzip2 is " << std::fixed << std::setprecision(1) 
                      << (result.speedup_mean - 1.0) * 100 << "% faster)";
        } else {
            std::cout << " (your BWT is " << std::fixed << std::setprecision(1) 
                      << (1.0 / result.speedup_mean - 1.0) * 100 << "% faster)";
        }
        std::cout << std::endl;
        
        double time_diff = result.your_time_mean - result.bzip2_time_mean;
        std::cout << "  Time Diff: " << std::fixed << std::setprecision(3) 
                  << time_diff << " ms";
        if (time_diff > 0) {
            std::cout << " (bzip2 is faster)";
        } else {
            std::cout << " (your BWT is faster)";
        }
        std::cout << std::endl;
        
        // Throughput comparison
        double your_throughput = (result.file_size / (1024.0 * 1024.0)) / (result.your_time_mean / 1000.0);
        double bzip2_throughput = (result.file_size / (1024.0 * 1024.0)) / (result.bzip2_time_mean / 1000.0);
        
        std::cout << "  Throughput:" << std::endl;
        std::cout << "    Your BWT:  " << std::fixed << std::setprecision(2) 
                  << your_throughput << " MB/s" << std::endl;
        std::cout << "    bzip2 BWT: " << std::fixed << std::setprecision(2) 
                  << bzip2_throughput << " MB/s" << std::endl;
    }
    
    std::cout << std::string(80, '=') << std::endl;
    
    // Output summary line for aggregation (format: SUMMARY|test_name|your_time_mean|bzip2_time_mean|speedup|winner|faster_by_pct)
    // Note: speedup = bzip2_time / your_time
    //       If speedup < 1.0, bzip2 is faster (took less time)
    //       If speedup > 1.0, your BWT is faster (took less time)
    if (result.speedup_mean > 0) {
        std::string winner = (result.speedup_mean < 1.0) ? "bzip2" : "your_bwt";
        double speedup_pct;
        if (result.speedup_mean < 1.0) {
            // bzip2 is faster: calculate how much faster
            speedup_pct = (1.0 / result.speedup_mean - 1.0) * 100.0;
        } else {
            // your BWT is faster: calculate how much faster
            speedup_pct = (result.speedup_mean - 1.0) * 100.0;
        }
        std::cout << "SUMMARY|" << result.test_name << "|" 
                  << std::fixed << std::setprecision(3) << result.your_time_mean << "|"
                  << std::fixed << std::setprecision(3) << result.bzip2_time_mean << "|"
                  << std::fixed << std::setprecision(3) << result.speedup_mean << "|"
                  << winner << "|"
                  << std::fixed << std::setprecision(1) << speedup_pct << std::endl;
    }
}

int main(int argc, char* argv[]) {
    std::cout << "\n" << std::string(80, '=') << std::endl;
    std::cout << "BWT Performance Comparison: Your Implementation vs bzip2" << std::endl;
    std::cout << std::string(80, '=') << std::endl;
    
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <input_file> [block_size]" << std::endl;
        std::cerr << "  input_file: File to test" << std::endl;
        std::cerr << "  block_size: Block size in bytes (default: 65536)" << std::endl;
        return 1;
    }
    
    std::string input_file = argv[1];
    size_t block_size = 65536;
    
    if (argc >= 3) {
        block_size = std::stoul(argv[2]);
    }
    
    // Check if input file exists
    if (!file_exists(input_file)) {
        std::cerr << "Error: Input file not found: " << input_file << std::endl;
        return 1;
    }
    
    // Check if bzip2 extractor exists
    if (!file_exists("build/bzip2_bwt_extractor")) {
        std::cerr << "Error: bzip2_bwt_extractor not found. Please compile it first." << std::endl;
        std::cerr << "  Run: make build/bzip2_bwt_extractor" << std::endl;
        return 1;
    }
    
    // Create temp directory
    if (!create_directory("build/tmp")) {
        std::cerr << "Error: Failed to create build/tmp directory" << std::endl;
        return 1;
    }
    
    // Extract test name from file path
    std::string test_name = input_file;
    size_t last_slash = test_name.find_last_of("/\\");
    if (last_slash != std::string::npos) {
        test_name = test_name.substr(last_slash + 1);
    }
    
    // Run comparison with 5 trials
    std::cout << "Running " << DEFAULT_NUM_TRIALS << " trial(s)..." << std::endl;
    ComparisonResult result = compare_implementations(input_file, test_name, block_size, DEFAULT_NUM_TRIALS);
    
    // Print results
    print_comparison(result);
    
    return 0;
}

