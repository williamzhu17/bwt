#include <iostream>
#include <cassert>
#include <string>
#include <vector>
#include <fstream>
#include <cstdio>
#include <algorithm>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include "../src/file_processor.hpp"
#include "../src/bwt.hpp"
#include "../src/inverse_bwt.hpp"

// Helper function to create directory if it doesn't exist
bool create_directory(const std::string& dir_path) {
    struct stat st;
    if (stat(dir_path.c_str(), &st) == 0) {
        // Directory already exists
        return S_ISDIR(st.st_mode);
    }
    
    // Create directory with rwxr-xr-x permissions
    return mkdir(dir_path.c_str(), 0755) == 0;
}

// Test result structure
struct TestResult {
    bool passed;
    std::string error_msg;
    
    TestResult(bool p, const std::string& msg = "") : passed(p), error_msg(msg) {}
};

// Medium test case structure
struct MediumTestCase {
    std::string name;
    std::string input_file;
    size_t block_size;
    char delimiter;
};

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

// Helper function to compare two files byte by byte
bool files_are_identical(const std::string& file1, const std::string& file2) {
    std::ifstream f1(file1, std::ios::binary);
    std::ifstream f2(file2, std::ios::binary);
    
    if (!f1.is_open() || !f2.is_open()) {
        return false;
    }
    
    // Compare byte by byte
    char c1, c2;
    while (true) {
        bool has1 = (bool)f1.get(c1);
        bool has2 = (bool)f2.get(c2);
        
        // If both reached EOF at the same time, files are identical
        if (!has1 && !has2) {
            return true;
        }
        
        // If only one reached EOF, files differ in length
        if (!has1 || !has2) {
            return false;
        }
        
        // If bytes differ, files are different
        if (c1 != c2) {
            return false;
        }
    }
}

// Test function for file-based BWT round-trip
TestResult test_file_round_trip(const std::string& test_name, const std::string& input_file, 
                                size_t block_size, char delimiter = '~') {
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
    
    // Step 1: Forward BWT (read chunks, transform, write)
    {
        FileProcessor processor(input_file, forward_file, block_size);
        if (!processor.is_open()) {
            return TestResult(false, "Failed to open files for forward BWT");
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
    
    // Step 2: Inverse BWT (read transformed chunks, inverse transform, write)
    {
        // Note: block_size + 1 because forward BWT adds a delimiter to each chunk
        FileProcessor processor(forward_file, recovered_file, block_size + 1);
        if (!processor.is_open()) {
            std::remove(forward_file.c_str());
            return TestResult(false, "Failed to open files for inverse BWT");
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
        // Skip "." and ".." entries
        if (std::string(entry->d_name) == "." || std::string(entry->d_name) == "..") {
            continue;
        }
        
        // Build full path to check if it's a file
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
std::vector<MediumTestCase> generate_test_cases(const std::string& data_dir, 
                                                 const std::vector<size_t>& block_sizes,
                                                 char delimiter = '~') {
    std::vector<MediumTestCase> test_cases;
    std::vector<std::string> files = list_files_in_directory(data_dir);
    
    // Sort files for consistent test ordering
    std::sort(files.begin(), files.end());
    
    for (const auto& filename : files) {
        for (size_t block_size : block_sizes) {
            std::string test_name = filename + " (" + std::to_string(block_size);
            if (block_size >= 1024) {
                test_name = filename + " (" + std::to_string(block_size / 1024) + "KB";
            }
            test_name += " blocks)";
            
            std::string file_path = data_dir + "/" + filename;
            test_cases.push_back({test_name, file_path, block_size, delimiter});
        }
    }
    
    return test_cases;
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
    std::string data_dir = "../data/medium_size";
    // std::vector<size_t> block_sizes = {128, 256, 512, 1024, 4096, 16384};
    std::vector<size_t> block_sizes = {128};
    
    // Dynamically generate test cases for all files in the directory
    std::cout << "Scanning directory: " << data_dir << std::endl;
    std::vector<MediumTestCase> test_cases = generate_test_cases(data_dir, block_sizes, '~');
    
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
                                                 test_case.block_size, 
                                                 test_case.delimiter);
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

