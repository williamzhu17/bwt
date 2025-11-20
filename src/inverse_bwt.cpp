#include "inverse_bwt.hpp"
#include "file_processor.hpp"
#include <iostream>
#include <vector>
#include <algorithm>
#include <string>
#include <fstream>
#include <cstring>

// Inverse BWT transform
std::string bwt_inverse(const std::string& bwt_str, char delimiter) {
    size_t len = bwt_str.length();
    
    // Initialize rotation_list with sorted single characters
    std::vector<std::string> rotation_list;

    for (size_t i = 0; i < len; i++) {
        rotation_list.push_back("");
    }
    
    // Repeat N-1 times: prepend BWT characters and sort
    for (size_t i = 0; i < len; i++) {
        for (size_t j = 0; j < len; j++) {
            rotation_list[j] = bwt_str[j] + rotation_list[j];
        }
        // Sort the strings
        std::sort(rotation_list.begin(), rotation_list.end());
    }
    
    // Find the string that ends with delimiter
    std::string output_str;
    for (const auto& str : rotation_list) {
        if (str.back() == delimiter) {
            // Return string without the delimiter
            output_str = str.substr(0, str.length() - 1);
            break;
        }
    }
    
    return output_str;
}

#ifndef BUILD_TESTS
int main(int argc, char* argv[]) {
    // Check for command line arguments
    if (argc < 3 || argc > 4) {
        std::cerr << "Usage: " << argv[0] << " <input_file> <output_file> [block_size]" << std::endl;
        std::cerr << "  block_size: size of each block in bytes (default: 128)" << std::endl;
        return 1;
    }
    
    // Parse block size (default 128B)
    size_t block_size = 128;
    if (argc == 4) {
        block_size = std::stoul(argv[3]);
        if (block_size == 0) {
            std::cerr << "Error: Block size must be greater than 0" << std::endl;
            return 1;
        }
    }
    
    // Note: Forward BWT outputs chunks of size (input_size + 1) due to delimiter
    // So we need to read chunks of size (block_size + 1) to match
    size_t bwt_chunk_size = block_size + 1;
    
    // Create FileProcessor to handle file I/O
    FileProcessor processor(argv[1], argv[2], bwt_chunk_size);
    
    if (!processor.is_open()) {
        return 1;
    }
    
    // Process file in chunks
    char delimiter = '~';
    
    while (processor.has_more_data()) {
        // Read a chunk (size block_size + 1 to account for delimiter)
        std::string chunk = processor.read_chunk();
        
        if (chunk.empty()) {
            break;
        }
        
        // Apply inverse BWT and write result
        std::string result = bwt_inverse(chunk, delimiter);
        processor.write_chunk(result);
    }
    
    processor.close();
    return 0;
}
#endif // BUILD_TESTS
