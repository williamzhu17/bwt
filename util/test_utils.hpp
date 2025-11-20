#ifndef TEST_UTILS_HPP
#define TEST_UTILS_HPP

#include <string>
#include <vector>

/**
 * Test utility structures and functions for BWT testing.
 */

/**
 * Test case structure for file-based BWT tests.
 * Used by both correctness tests and performance benchmarks.
 */
struct FileTestCase {
    std::string name;
    std::string input_file;
    size_t block_size;
};

/**
 * Generates test cases for all files in a directory with specified block sizes.
 * @param data_dir Path to directory containing test files
 * @param block_sizes Vector of block sizes to test
 * @param verbose_names If true, includes block size in test name (e.g., "file.txt (128KB blocks)")
 * @return Vector of FileTestCase objects
 */
std::vector<FileTestCase> generate_file_test_cases(const std::string& data_dir,
                                                    const std::vector<size_t>& block_sizes,
                                                    bool verbose_names = false);

#endif // TEST_UTILS_HPP

