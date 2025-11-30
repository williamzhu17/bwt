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
#include "../src/file_processor.hpp"
#include "../src/bwt.hpp"
#include "../util/file_utils.hpp"
#include "../util/format_utils.hpp"

struct ComparisonResult {
    std::string test_name;
    size_t block_size;
    size_t file_size;
    
    // Your BWT results
    double your_time = 0.0;
    size_t your_output_size = 0;
    
    // bzip2 BWT results
    double bzip2_time = 0.0;
    size_t bzip2_output_size = 0;
    
    // Comparison
    double speedup = 0.0;  // bzip2_time / your_time (if > 1, bzip2 is faster)
    double time_diff = 0.0;
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

// Compare implementations on a single file
ComparisonResult compare_implementations(const std::string& input_file, 
                                         const std::string& test_name,
                                         size_t block_size) {
    ComparisonResult result;
    result.test_name = test_name;
    result.block_size = block_size;
    result.file_size = get_file_size(input_file);
    
    std::string your_output = "build/tmp/your_bwt_output.bwt";
    std::string bzip2_output = "build/tmp/bzip2_bwt_output.bwt";
    
    // Clean up previous outputs
    std::remove(your_output.c_str());
    std::remove(bzip2_output.c_str());
    
    // Run your BWT
    bool your_success = run_your_bwt(input_file, your_output, block_size, 
                                     result.your_time, result.your_output_size);
    
    // Run bzip2 BWT
    bool bzip2_success = run_bzip2_bwt(input_file, bzip2_output, block_size,
                                       result.bzip2_time, result.bzip2_output_size);
    
    if (!your_success) {
        std::cerr << "Warning: Your BWT failed for " << test_name << std::endl;
    }
    
    if (!bzip2_success) {
        std::cerr << "Warning: bzip2 BWT failed for " << test_name << std::endl;
    }
    
    // Calculate comparison metrics
    if (your_success && bzip2_success) {
        if (result.your_time > 0) {
            result.speedup = result.bzip2_time / result.your_time;
        }
        result.time_diff = result.your_time - result.bzip2_time;
    }
    
    // Clean up
    std::remove(your_output.c_str());
    std::remove(bzip2_output.c_str());
    
    return result;
}

// Print comparison results
void print_comparison(const ComparisonResult& result) {
    std::cout << "\n" << std::string(80, '=') << std::endl;
    std::cout << "Test: " << result.test_name << std::endl;
    std::cout << "Block Size: " << format_size(result.block_size) << std::endl;
    std::cout << "File Size: " << format_size(result.file_size) << std::endl;
    std::cout << std::string(80, '-') << std::endl;
    
    std::cout << "Your BWT:" << std::endl;
    std::cout << "  Time:      " << std::fixed << std::setprecision(3) 
              << result.your_time << " ms" << std::endl;
    std::cout << "  Output:    " << format_size(result.your_output_size) << std::endl;
    
    std::cout << "\nbzip2 BWT:" << std::endl;
    std::cout << "  Time:      " << std::fixed << std::setprecision(3) 
              << result.bzip2_time << " ms" << std::endl;
    std::cout << "  Output:    " << format_size(result.bzip2_output_size) << std::endl;
    
    if (result.speedup > 0) {
        std::cout << "\nComparison:" << std::endl;
        std::cout << "  Speedup:   " << std::fixed << std::setprecision(3) 
                  << result.speedup << "x";
        if (result.speedup > 1.0) {
            std::cout << " (bzip2 is " << std::fixed << std::setprecision(1) 
                      << (result.speedup - 1.0) * 100 << "% faster)";
        } else {
            std::cout << " (your BWT is " << std::fixed << std::setprecision(1) 
                      << (1.0 / result.speedup - 1.0) * 100 << "% faster)";
        }
        std::cout << std::endl;
        
        std::cout << "  Time Diff: " << std::fixed << std::setprecision(3) 
                  << result.time_diff << " ms";
        if (result.time_diff > 0) {
            std::cout << " (bzip2 is faster)";
        } else {
            std::cout << " (your BWT is faster)";
        }
        std::cout << std::endl;
        
        // Throughput comparison
        double your_throughput = (result.file_size / (1024.0 * 1024.0)) / (result.your_time / 1000.0);
        double bzip2_throughput = (result.file_size / (1024.0 * 1024.0)) / (result.bzip2_time / 1000.0);
        
        std::cout << "  Throughput:" << std::endl;
        std::cout << "    Your BWT:  " << std::fixed << std::setprecision(2) 
                  << your_throughput << " MB/s" << std::endl;
        std::cout << "    bzip2 BWT: " << std::fixed << std::setprecision(2) 
                  << bzip2_throughput << " MB/s" << std::endl;
    }
    
    std::cout << std::string(80, '=') << std::endl;
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
        std::cerr << "  See README_BWT_COMPARISON.md for instructions." << std::endl;
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
    
    // Run comparison
    ComparisonResult result = compare_implementations(input_file, test_name, block_size);
    
    // Print results
    print_comparison(result);
    
    return 0;
}

