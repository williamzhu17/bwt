#include "bwt.hpp"
#include "file_processor.hpp"
#include <iostream>
#include <vector>
#include <algorithm>
#include <string>
#include <fstream>
#include <sstream>
#include <cstring>

// Forward BWT transform
std::string bwt_forward(const std::string& input, char delimiter) {
    // Add delimiter to input string
    std::string cur_str = input + delimiter;
    size_t len = cur_str.length();
    // Generate all rotations
    std::vector<std::string> rotations;
    for (size_t i = 0; i < len; i++) {
        // Cyclic shift: take last char and put it at front
        cur_str = cur_str.back() + cur_str.substr(0, len - 1);
        rotations.push_back(cur_str);
    }
    
    // Sort rotations
    std::sort(rotations.begin(), rotations.end());
    
    // Extract last character of each sorted rotation
    std::string bwt_str;
    for (const auto& s : rotations) {
        bwt_str += s.back();
    }
    
    return bwt_str;
}

#ifndef BUILD_TESTS
int main(int argc, char* argv[]) {
    // Check for command line arguments
    if (argc < 3 || argc > 4) {
        std::cerr << "Usage: " << argv[0] << " <input_file> <output_file> [block_size]" << std::endl;
        std::cerr << "  block_size: size of each block in bytes (default: 65536)" << std::endl;
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
    
    // Create FileProcessor to handle file I/O
    FileProcessor processor(argv[1], argv[2], block_size);
    
    if (!processor.is_open()) {
        return 1;
    }
    
    // Process file in chunks
    char delimiter = '~';
    
    while (processor.has_more_data()) {
        // Read a chunk
        std::string chunk = processor.read_chunk();
        
        if (chunk.empty()) {
            break;
        }
        
        // Apply BWT and write result
        std::string result = bwt_forward(chunk, delimiter);
        processor.write_chunk(result);
    }
    
    processor.close();
    return 0;
}
#endif // BUILD_TESTS
