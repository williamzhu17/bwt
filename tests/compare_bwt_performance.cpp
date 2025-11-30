/*
 * Performance comparison between your BWT and bzip2's BWT
 * 
 * This program runs both implementations and compares their performance
 */

#include <iostream>
#include <string>
#include <vector>
#include <iomanip>
#include "../src/file_processor.hpp"
#include "../src/bwt.hpp"
#include "../src/inverse_bwt.hpp"
#include "../util/file_utils.hpp"
#include "../util/format_utils.hpp"
#include "../util/performance_comparison.hpp"
#include "../util/bwt_benchmark_runner.hpp"

// Configuration
const int DEFAULT_NUM_TRIALS = 5;

// Default block sizes for performance tests
// Note: bzip2 allocates space in multiples of 100KB, but can process any size up to the allocated maximum
// The conversion logic automatically calculates the minimum bzip2 internal block size parameter needed
const std::vector<size_t> DEFAULT_BLOCK_SIZES = {65536, 131072, 262144};  // 64KB, 128KB, 256KB

/**
 * Create temporary file paths for a trial
 */
static TrialTempFiles create_temp_files() {
    TrialTempFiles files;
    files.your_forward_output = "build/tmp/your_forward.bwt";
    files.your_inverse_output = "build/tmp/your_inverse.txt";
    files.bzip2_forward_output = "build/tmp/bzip2_forward.bwt";
    files.bzip2_inverse_output = "build/tmp/bzip2_inverse.txt";
    return files;
}

/**
 * Extract test name from file path
 */
static std::string extract_test_name(const std::string& file_path) {
    std::string test_name = file_path;
    size_t last_slash = test_name.find_last_of("/\\");
    if (last_slash != std::string::npos) {
        test_name = test_name.substr(last_slash + 1);
    }
    return test_name;
}

/**
 * Compare implementations on a single file with multiple trials
 * @param input_file Input file path
 * @param test_name Test name
 * @param block_size Block size for processing
 * @param num_trials Number of trials to run
 * @return Comparison result with statistics
 */
static ComparisonResult compare_implementations(const std::string& input_file,
                                                 const std::string& test_name,
                                                 size_t block_size,
                                                 int num_trials) {
    ComparisonResult result;
    result.test_name = test_name;
    result.block_size = block_size;
    result.file_size = get_file_size(input_file);
    result.num_trials = num_trials;
    
    TrialTempFiles temp_files = create_temp_files();
    
    // Run multiple trials
    for (int trial = 0; trial < num_trials; trial++) {
        TrialResult trial_result;
        
        if (!BWTBenchmarkRunner::run_single_trial(input_file, block_size, temp_files, trial_result)) {
            std::cerr << "Warning: Trial " << (trial + 1) << " failed for " << test_name << std::endl;
            continue;
        }
        
        result.trials.push_back(trial_result);
    }
    
    // Calculate statistics
    result.calculate_statistics();
    
    // Clean up
    temp_files.cleanup();
    
    return result;
}

/**
 * Validate input file and create necessary directories
 */
static bool validate_and_setup(const std::string& input_file) {
    // Check if input file exists
    if (!file_exists(input_file)) {
        std::cerr << "Error: Input file not found: " << input_file << std::endl;
        return false;
    }
    
    // Create temp directory
    if (!create_directory("build/tmp")) {
        std::cerr << "Error: Failed to create build/tmp directory" << std::endl;
        return false;
    }
    
    return true;
}

/**
 * Print program header
 */
static void print_header() {
    std::cout << "\n" << std::string(80, '=') << std::endl;
    std::cout << "BWT Performance Comparison: Your Implementation vs bzip2" << std::endl;
    std::cout << "Testing: Forward BWT, Inverse BWT, and Round Trip" << std::endl;
    std::cout << std::string(80, '=') << std::endl;
}

/**
 * Print usage information
 */
static void print_usage(const char* program_name) {
    std::cerr << "Usage: " << program_name << " <input_file>" << std::endl;
    std::cerr << "  input_file: File to test" << std::endl;
    std::cerr << "  Note: Using default block sizes: 64KB, 128KB, 256KB" << std::endl;
}

int main(int argc, char* argv[]) {
    print_header();
    
    if (argc < 2) {
        print_usage(argv[0]);
        return 1;
    }
    
    std::string input_file = argv[1];
    
    // Validate input and setup
    if (!validate_and_setup(input_file)) {
        return 1;
    }
    
    // Extract test name from file path
    std::string test_name = extract_test_name(input_file);
    
    // Run comparison for each default block size
    for (size_t block_size : DEFAULT_BLOCK_SIZES) {
        std::cout << "\n" << std::string(80, '=') << std::endl;
        std::cout << "Block Size: " << format_size(block_size) << std::endl;
        std::cout << std::string(80, '=') << std::endl;
        
        std::cout << "Running " << DEFAULT_NUM_TRIALS << " trial(s)..." << std::endl;
        ComparisonResult result = compare_implementations(input_file, test_name, block_size, DEFAULT_NUM_TRIALS);
        
        // Print results
        ComparisonPrinter::print_comparison(result);
    }
    
    return 0;
}
