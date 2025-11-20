#include "test_utils.hpp"
#include "file_utils.hpp"
#include <algorithm>

std::vector<FileTestCase> generate_file_test_cases(const std::string& data_dir,
                                                    const std::vector<size_t>& block_sizes,
                                                    bool verbose_names) {
    std::vector<FileTestCase> test_cases;
    std::vector<std::string> files = list_files_in_directory(data_dir);
    
    // Sort files for consistent test ordering
    std::sort(files.begin(), files.end());
    
    for (const auto& filename : files) {
        for (size_t block_size : block_sizes) {
            std::string test_name;
            
            if (verbose_names) {
                // Verbose format: "filename (128 blocks)" or "filename (1KB blocks)"
                test_name = filename + " (" + std::to_string(block_size);
                if (block_size >= 1024) {
                    test_name = filename + " (" + std::to_string(block_size / 1024) + "KB";
                }
                test_name += " blocks)";
            } else {
                // Simple format: just the filename
                test_name = filename;
            }
            
            std::string file_path = data_dir + "/" + filename;
            test_cases.push_back({test_name, file_path, block_size});
        }
    }
    
    return test_cases;
}

