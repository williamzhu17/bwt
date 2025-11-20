#include <iostream>
#include <cassert>
#include <string>
#include <vector>
#include <fstream>
#include <cstdio>
#include <algorithm>
#include "../src/file_processor.hpp"
#include "../src/bwt.hpp"
#include "../src/inverse_bwt.hpp"
#include "../util/file_utils.hpp"
#include "../util/test_utils.hpp"

// Test result structure
struct TestResult {
    bool passed;
    std::string error_msg;
    
    TestResult(bool p, const std::string& msg = "") : passed(p), error_msg(msg) {}
};

// Test function for file-based BWT round-trip
TestResult test_file_round_trip(const std::string& test_name, const std::string& input_file, 
                                size_t block_size) {
    // Check if input file exists
    if (!file_exists(input_file)) {
        return TestResult(false, "Input file does not exist: " + input_file);
    }
    
    size_t original_size = get_file_size(input_file);
    if (original_size == 0) {
        return TestResult(false, "Input file is empty or cannot be read");
    }
    
    // Create unique filenames based on test name
    // Replace spaces with underscores for filename safety
    std::string safe_name = test_name;
    for (char& c : safe_name) {
        if (c == ' ' || c == '(' || c == ')') {
            c = '_';
        }
    }
    
    // Write temporary files to tmp/ directory
    std::string forward_file = "tmp/" + safe_name + "_forward";
    std::string recovered_file = "tmp/" + safe_name + "_recovered";
    
    // Step 1: Forward BWT using process_file from bwt.hpp
    // Forward BWT is defined in the bwt namespace/compilation unit
    int forward_result = bwt_forward_process_file(input_file.c_str(), forward_file.c_str(), block_size);
    if (forward_result != 0) {
        return TestResult(false, "Failed to process forward BWT");
    }
    
    // Verify forward file was created and has correct size
    if (!file_exists(forward_file)) {
        return TestResult(false, "Forward BWT output file was not created");
    }
    
    size_t forward_size = get_file_size(forward_file);
    size_t expected_forward_size = original_size;
    // Calculate expected size: each block adds 1 delimiter
    size_t num_blocks = (original_size + block_size - 1) / block_size;
    expected_forward_size += num_blocks;
    
    if (forward_size != expected_forward_size) {
        std::remove(forward_file.c_str());
        return TestResult(false, "Forward BWT output size mismatch: expected " + 
                          std::to_string(expected_forward_size) + ", got " + 
                          std::to_string(forward_size));
    }
    
    // Step 2: Inverse BWT using process_file from inverse_bwt.hpp
    int inverse_result = bwt_inverse_process_file(forward_file.c_str(), recovered_file.c_str(), block_size);
    if (inverse_result != 0) {
        std::remove(forward_file.c_str());
        return TestResult(false, "Failed to process inverse BWT");
    }
    
    // Verify recovered file was created
    if (!file_exists(recovered_file)) {
        std::remove(forward_file.c_str());
        return TestResult(false, "Inverse BWT output file was not created");
    }
    
    // Verify size matches original
    size_t recovered_size = get_file_size(recovered_file);
    if (recovered_size != original_size) {
        std::remove(forward_file.c_str());
        std::remove(recovered_file.c_str());
        return TestResult(false, "Recovered file size mismatch: expected " + 
                          std::to_string(original_size) + ", got " + 
                          std::to_string(recovered_size));
    }
    
    // Step 3: Verify round-trip (compare original with recovered)
    bool identical = files_are_identical(input_file, recovered_file);

    if (!identical) {
        return TestResult(false, "Round-trip failed: recovered file differs from original");
    }
    
    return TestResult(true);
}

// Helper function to run a test and report results
void run_test(const std::string& test_name, const TestResult& result) {
    std::cout << "[" << (result.passed ? "PASS" : "FAIL") << "] " << test_name;
    if (!result.passed && !result.error_msg.empty()) {
        std::cout << "\n    Error: " << result.error_msg;
    }
    std::cout << std::endl;
}

int main() {
    std::cout << "Running BWT medium file tests...\n" << std::endl;
    
    // Create tmp directory for test outputs
    if (!create_directory("tmp")) {
        std::cerr << "Error: Failed to create tmp directory" << std::endl;
        return 1;
    }
    std::cout << "Output directory: tmp/" << std::endl;
    std::cout << "All forward and recovered files will be saved for inspection.\n" << std::endl;
    
    // Define the data directory and block sizes to test
    std::string data_dir = "../data/canterbury_corpus";
    // std::vector<size_t> block_sizes = {128, 256, 512, 1024, 4096, 16384};
    std::vector<size_t> block_sizes = {128};
    
    // Dynamically generate test cases for all files in the directory
    std::cout << "Scanning directory: " << data_dir << std::endl;
    std::vector<FileTestCase> test_cases = generate_file_test_cases(data_dir, block_sizes, true);
    
    if (test_cases.empty()) {
        std::cerr << "Error: No test cases generated. Check if data directory exists and contains files." << std::endl;
        return 1;
    }
    
    std::cout << "Generated " << test_cases.size() << " test cases from " 
              << (test_cases.size() / block_sizes.size()) << " files\n" << std::endl;
    
    // Run all tests and track results
    int passed_count = 0;
    int failed_count = 0;
    
    for (const auto& test_case : test_cases) {
        std::cout << "Running: " << test_case.name << std::endl;
        TestResult result = test_file_round_trip(test_case.name,
                                                 test_case.input_file, 
                                                 test_case.block_size);
        run_test(test_case.name, result);
        
        if (result.passed) {
            passed_count++;
        } else {
            failed_count++;
        }
    }
    
    // Print summary
    std::cout << "\n=== Test Summary ===" << std::endl;
    std::cout << "Total tests: " << test_cases.size() << std::endl;
    std::cout << "Passed: " << passed_count << std::endl;
    std::cout << "Failed: " << failed_count << std::endl;
    
    if (failed_count == 0) {
        std::cout << "\nAll tests passed!" << std::endl;
        return 0;
    } else {
        std::cout << "\nSome tests failed!" << std::endl;
        return 1;
    }
}

