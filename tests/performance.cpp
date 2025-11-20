#include <iostream>
#include <string>
#include <vector>
#include <cstdio>
#include <algorithm>
#include <chrono>
#include <numeric>
#include <cmath>
#include "../src/file_processor.hpp"
#include "../src/bwt.hpp"
#include "../src/inverse_bwt.hpp"
#include "../util/file_utils.hpp"
#include "../util/format_utils.hpp"
#include "../util/test_utils.hpp"

// Configuration
const int DEFAULT_NUM_TRIALS = 5;

// Performance metrics structure
struct PerformanceMetrics {
    std::vector<double> forward_times;
    std::vector<double> inverse_times;
    std::vector<double> total_times;
    
    double forward_mean;
    double forward_stddev;
    double inverse_mean;
    double inverse_stddev;
    double total_mean;
    double total_stddev;
    
    size_t input_size;
    size_t output_size;
    
    void calculate_statistics() {
        forward_mean = calculate_mean(forward_times);
        forward_stddev = calculate_stddev(forward_times, forward_mean);
        inverse_mean = calculate_mean(inverse_times);
        inverse_stddev = calculate_stddev(inverse_times, inverse_mean);
        total_mean = calculate_mean(total_times);
        total_stddev = calculate_stddev(total_times, total_mean);
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

// Performance test function - runs forward and inverse BWT and measures time
PerformanceMetrics run_performance_test(const std::string& input_file, 
                                        size_t block_size, 
                                        int num_trials) {
    PerformanceMetrics metrics;
    metrics.input_size = get_file_size(input_file);
    
    // Generate temporary file names
    std::string forward_file = "build/tmp/perf_temp.bwt";
    std::string recovered_file = "build/tmp/perf_recovered.txt";
    
    // Run multiple trials
    for (int trial = 0; trial < num_trials; trial++) {
        // Clean up previous files
        std::remove(forward_file.c_str());
        std::remove(recovered_file.c_str());
        
        // Forward BWT with timing
        auto forward_start = std::chrono::high_resolution_clock::now();
        int forward_result = bwt_forward_process_file(input_file.c_str(), forward_file.c_str(), block_size);
        auto forward_end = std::chrono::high_resolution_clock::now();
        
        if (forward_result != 0) {
            std::cerr << "Failed to process forward BWT" << std::endl;
            return metrics;
        }
        
        // Record forward time
        std::chrono::duration<double> forward_duration = forward_end - forward_start;
        metrics.forward_times.push_back(forward_duration.count());
        
        // Get output size (only once)
        if (trial == 0) {
            metrics.output_size = get_file_size(forward_file);
        }
        
        // Inverse BWT with timing
        auto inverse_start = std::chrono::high_resolution_clock::now();
        int inverse_result = bwt_inverse_process_file(forward_file.c_str(), recovered_file.c_str(), block_size);
        auto inverse_end = std::chrono::high_resolution_clock::now();
        
        if (inverse_result != 0) {
            std::cerr << "Failed to process inverse BWT" << std::endl;
            return metrics;
        }
        
        // Record inverse time
        std::chrono::duration<double> inverse_duration = inverse_end - inverse_start;
        metrics.inverse_times.push_back(inverse_duration.count());
        
        // Record total time
        metrics.total_times.push_back(forward_duration.count() + inverse_duration.count());
    }
    
    // Clean up temporary files
    std::remove(forward_file.c_str());
    std::remove(recovered_file.c_str());
    
    // Calculate statistics
    metrics.calculate_statistics();
    
    return metrics;
}

// Print performance results
void print_performance_results(const std::string& test_name, 
                               const PerformanceMetrics& metrics,
                               size_t block_size) {
    std::cout << "\n" << std::string(70, '=') << std::endl;
    std::cout << "Test: " << test_name << std::endl;
    std::cout << "Block Size: " << format_size(block_size) << std::endl;
    std::cout << "Input Size: " << format_size(metrics.input_size) << std::endl;
    std::cout << "Output Size: " << format_size(metrics.output_size) << std::endl;
    
    double compression_ratio = (metrics.input_size > 0) ? 
        (double)metrics.output_size / metrics.input_size : 0.0;
    std::cout << "Compression Ratio: " << std::fixed << std::setprecision(4) 
              << compression_ratio << std::endl;
    
    std::cout << std::string(70, '-') << std::endl;
    std::cout << "Trials: " << metrics.forward_times.size() << std::endl;
    std::cout << std::string(70, '-') << std::endl;
    
    // Forward BWT results
    std::cout << "Forward BWT:" << std::endl;
    std::cout << "  Mean:   " << format_time(metrics.forward_mean);
    if (metrics.forward_times.size() > 1) {
        std::cout << " ± " << format_time(metrics.forward_stddev);
    }
    std::cout << std::endl;
    
    std::cout << "  Min:    " << format_time(*std::min_element(
        metrics.forward_times.begin(), metrics.forward_times.end())) << std::endl;
    std::cout << "  Max:    " << format_time(*std::max_element(
        metrics.forward_times.begin(), metrics.forward_times.end())) << std::endl;
    
    // Inverse BWT results
    std::cout << "\nInverse BWT:" << std::endl;
    std::cout << "  Mean:   " << format_time(metrics.inverse_mean);
    if (metrics.inverse_times.size() > 1) {
        std::cout << " ± " << format_time(metrics.inverse_stddev);
    }
    std::cout << std::endl;
    
    std::cout << "  Min:    " << format_time(*std::min_element(
        metrics.inverse_times.begin(), metrics.inverse_times.end())) << std::endl;
    std::cout << "  Max:    " << format_time(*std::max_element(
        metrics.inverse_times.begin(), metrics.inverse_times.end())) << std::endl;
    
    // Total roundtrip results
    std::cout << "\nTotal Roundtrip:" << std::endl;
    std::cout << "  Mean:   " << format_time(metrics.total_mean);
    if (metrics.total_times.size() > 1) {
        std::cout << " ± " << format_time(metrics.total_stddev);
    }
    std::cout << std::endl;
    
    std::cout << "  Min:    " << format_time(*std::min_element(
        metrics.total_times.begin(), metrics.total_times.end())) << std::endl;
    std::cout << "  Max:    " << format_time(*std::max_element(
        metrics.total_times.begin(), metrics.total_times.end())) << std::endl;
    
    // Throughput
    double throughput_mb_s = (metrics.input_size / (1024.0 * 1024.0)) / metrics.total_mean;
    std::cout << "\nThroughput: " << std::fixed << std::setprecision(2) 
              << throughput_mb_s << " MB/s" << std::endl;
    
    std::cout << std::string(70, '=') << std::endl;
}

int main(int argc, char* argv[]) {
    std::cout << "\n" << std::string(70, '=') << std::endl;
    std::cout << "BWT Performance Benchmark" << std::endl;
    std::cout << std::string(70, '=') << std::endl;
    
    // Default configuration
    std::string data_dir = "data/canterbury_corpus";
    int num_trials = DEFAULT_NUM_TRIALS;

    // Parse command line arguments
    if (argc > 1) {
        data_dir = argv[1];
    }
    
    if (argc > 2) {
        num_trials = std::atoi(argv[2]);
        if (num_trials < 1) {
            std::cerr << "Invalid number of trials. Using default: " << DEFAULT_NUM_TRIALS << std::endl;
            num_trials = DEFAULT_NUM_TRIALS;
        }
    }
    
    std::cout << "Dataset Directory: " << data_dir << std::endl;
    std::cout << "Number of trials per test: " << num_trials << std::endl;
    
    // Create build/tmp directory for temporary files
    if (!create_directory("build/tmp")) {
        std::cerr << "Error: Failed to create build/tmp directory" << std::endl;
        return 1;
    }
    
    // Define block sizes to test
    std::vector<size_t> block_sizes = {
        512,        // 512 bytes
        1 * 1024,   // 1 KB
        4 * 1024,   // 4 KB
        16 * 1024,  // 16 KB
        64 * 1024  // 64 KB
        //256 * 1024  // 256 KB // too large for my computer
    };
    
    // Check if data directory exists
    if (!directory_exists(data_dir)) {
        std::cerr << "Error: Data directory not found: " << data_dir << std::endl;
        return 1;
    }
    
    // Generate test cases
    std::cout << "Scanning directory: " << data_dir << std::endl;
    std::vector<FileTestCase> test_cases = generate_file_test_cases(data_dir, block_sizes, false);
    
    if (test_cases.empty()) {
        std::cerr << "Error: No test cases generated. Check if data directory contains files." << std::endl;
        return 1;
    }
    
    std::cout << "Found " << (test_cases.size() / block_sizes.size()) << " files" << std::endl;
    std::cout << "Testing " << block_sizes.size() << " block sizes" << std::endl;
    std::cout << "Total test cases: " << test_cases.size() << std::endl;
    std::cout << std::string(70, '=') << std::endl;
    
    // Run all performance tests
    int completed = 0;
    for (const auto& test_case : test_cases) {
        completed++;
        std::cout << "\n[" << completed << "/" << test_cases.size() << "] Running: " 
                  << test_case.name << " (block size: " << format_size(test_case.block_size) 
                  << ")" << std::endl;
        
        // Check if input file exists
        if (!file_exists(test_case.input_file)) {
            std::cerr << "Error: Input file not found: " << test_case.input_file << std::endl;
            continue;
        }
        
        // Run performance test
        PerformanceMetrics metrics = run_performance_test(
            test_case.input_file,
            test_case.block_size,
            num_trials
        );
        
        // Print results
        print_performance_results(test_case.name, metrics, test_case.block_size);
    }
    
    std::cout << "\n" << std::string(70, '=') << std::endl;
    std::cout << "Performance Benchmark Complete!" << std::endl;
    std::cout << "Total tests completed: " << completed << std::endl;
    std::cout << std::string(70, '=') << std::endl;
    
    return 0;
}

