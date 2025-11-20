#include <iostream>
#include <iomanip>
#include <sstream>
#include <string>
#include <vector>
#include <fstream>
#include <cstdio>
#include <algorithm>
#include <chrono>
#include <numeric>
#include <cmath>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include "../src/file_processor.hpp"
#include "../src/bwt.hpp"
#include "../src/inverse_bwt.hpp"

// Configuration
const int NUM_TRIALS = 5;  // Number of times to run each test

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

// Test case structure
struct PerformanceTestCase {
    std::string name;
    std::string input_file;
    size_t block_size;
    char delimiter;
};

// Helper function to create directory if it doesn't exist
bool create_directory(const std::string& dir_path) {
    struct stat st;
    if (stat(dir_path.c_str(), &st) == 0) {
        return S_ISDIR(st.st_mode);
    }
    return mkdir(dir_path.c_str(), 0755) == 0;
}

// Helper function to check if a file exists
bool file_exists(const std::string& filename) {
    struct stat buffer;
    return (stat(filename.c_str(), &buffer) == 0);
}

// Helper function to get file size
size_t get_file_size(const std::string& filename) {
    struct stat buffer;
    if (stat(filename.c_str(), &buffer) != 0) {
        return 0;
    }
    return buffer.st_size;
}

// Performance test function - runs forward and inverse BWT and measures time
PerformanceMetrics run_performance_test(const std::string& input_file, 
                                        size_t block_size, 
                                        char delimiter,
                                        int num_trials) {
    PerformanceMetrics metrics;
    metrics.input_size = get_file_size(input_file);
    
    // Generate temporary file names
    std::string forward_file = "tmp/perf_forward.bwt";
    std::string recovered_file = "tmp/perf_recovered.txt";
    
    // Run multiple trials
    for (int trial = 0; trial < num_trials; trial++) {
        // Clean up previous files
        std::remove(forward_file.c_str());
        std::remove(recovered_file.c_str());
        
        // Forward BWT with timing
        auto forward_start = std::chrono::high_resolution_clock::now();
        {
            FileProcessor processor(input_file, forward_file, block_size);
            if (!processor.is_open()) {
                std::cerr << "Failed to open files for forward BWT" << std::endl;
                return metrics;
            }
            
            while (processor.has_more_data()) {
                std::string chunk = processor.read_chunk();
                if (chunk.empty()) {
                    break;
                }
                
                std::string transformed = bwt_forward(chunk, delimiter);
                processor.write_chunk(transformed);
            }
        }
        auto forward_end = std::chrono::high_resolution_clock::now();
        
        // Record forward time
        std::chrono::duration<double> forward_duration = forward_end - forward_start;
        metrics.forward_times.push_back(forward_duration.count());
        
        // Get output size (only once)
        if (trial == 0) {
            metrics.output_size = get_file_size(forward_file);
        }
        
        // Inverse BWT with timing
        auto inverse_start = std::chrono::high_resolution_clock::now();
        {
            FileProcessor processor(forward_file, recovered_file, block_size + 1);
            if (!processor.is_open()) {
                std::cerr << "Failed to open files for inverse BWT" << std::endl;
                return metrics;
            }
            
            while (processor.has_more_data()) {
                std::string chunk = processor.read_chunk();
                if (chunk.empty()) {
                    break;
                }
                
                std::string recovered = bwt_inverse(chunk, delimiter);
                processor.write_chunk(recovered);
            }
        }
        auto inverse_end = std::chrono::high_resolution_clock::now();
        
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

// Helper function to format time with appropriate units
std::string format_time(double seconds) {
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(4);
    
    if (seconds < 0.001) {
        oss << (seconds * 1000000) << " μs";
    } else if (seconds < 1.0) {
        oss << (seconds * 1000) << " ms";
    } else {
        oss << seconds << " s";
    }
    
    return oss.str();
}

// Helper function to format file size
std::string format_size(size_t bytes) {
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(2);
    
    if (bytes < 1024) {
        oss << bytes << " B";
    } else if (bytes < 1024 * 1024) {
        oss << (bytes / 1024.0) << " KB";
    } else {
        oss << (bytes / (1024.0 * 1024.0)) << " MB";
    }
    
    return oss.str();
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

// Helper function to list all files in a directory
std::vector<std::string> list_files_in_directory(const std::string& dir_path) {
    std::vector<std::string> files;
    DIR* dir = opendir(dir_path.c_str());
    
    if (dir == nullptr) {
        std::cerr << "Warning: Could not open directory: " << dir_path << std::endl;
        return files;
    }
    
    struct dirent* entry;
    while ((entry = readdir(dir)) != nullptr) {
        if (std::string(entry->d_name) == "." || std::string(entry->d_name) == "..") {
            continue;
        }
        
        std::string full_path = dir_path + "/" + entry->d_name;
        struct stat st;
        if (stat(full_path.c_str(), &st) == 0 && S_ISREG(st.st_mode)) {
            files.push_back(entry->d_name);
        }
    }
    
    closedir(dir);
    return files;
}

// Generate test cases for all files in a directory with specified block sizes
std::vector<PerformanceTestCase> generate_test_cases(const std::string& data_dir, 
                                                     const std::vector<size_t>& block_sizes,
                                                     char delimiter = '~') {
    std::vector<PerformanceTestCase> test_cases;
    std::vector<std::string> files = list_files_in_directory(data_dir);
    
    // Sort files for consistent test ordering
    std::sort(files.begin(), files.end());
    
    for (const auto& filename : files) {
        for (size_t block_size : block_sizes) {
            std::string test_name = filename;
            std::string file_path = data_dir + "/" + filename;
            test_cases.push_back({test_name, file_path, block_size, delimiter});
        }
    }
    
    return test_cases;
}

int main(int argc, char* argv[]) {
    std::cout << "\n" << std::string(70, '=') << std::endl;
    std::cout << "BWT Performance Benchmark - Canterbury Corpus" << std::endl;
    std::cout << std::string(70, '=') << std::endl;
    
    // Parse command line arguments for number of trials
    int num_trials = NUM_TRIALS;
    if (argc > 1) {
        num_trials = std::atoi(argv[1]);
        if (num_trials < 1) {
            std::cerr << "Invalid number of trials. Using default: " << NUM_TRIALS << std::endl;
            num_trials = NUM_TRIALS;
        }
    }
    
    std::cout << "Number of trials per test: " << num_trials << std::endl;
    
    // Create tmp directory for temporary files
    if (!create_directory("tmp")) {
        std::cerr << "Error: Failed to create tmp directory" << std::endl;
        return 1;
    }
    
    // Define the data directory and block sizes to test
    std::string data_dir = "../data/canterbury_corpus";
    // std::vector<size_t> block_sizes = {128, 256, 512, 1024};
    std::vector<size_t> block_sizes = {128};
    
    // Check if data directory exists
    struct stat st;
    if (stat(data_dir.c_str(), &st) != 0 || !S_ISDIR(st.st_mode)) {
        std::cerr << "Error: Data directory not found: " << data_dir << std::endl;
        return 1;
    }
    
    // Generate test cases
    std::cout << "Scanning directory: " << data_dir << std::endl;
    std::vector<PerformanceTestCase> test_cases = generate_test_cases(data_dir, block_sizes, '~');
    
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
            test_case.delimiter,
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

