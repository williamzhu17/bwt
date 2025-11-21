#include "bwt.hpp"
#include "file_processor.hpp"
#include <iostream>
#include <vector>
#include <algorithm>
#include <string>
#include <fstream>
#include <sstream>
#include <cstring>
#include <unordered_set>

// Find a byte value (0-255) that does not appear in the file.
// Returns the first unused byte value, or -1 if all 256 values are used.
int find_unique_char(const char* file_path) {
    std::unordered_set<unsigned char> used_bytes;
    const size_t chunk_size = 8192;  // 8KB chunks
    
    FileProcessor processor(file_path, "", chunk_size);
    if (!processor.is_open()) {
        return -1;
    }
    
    // Read file in chunks and track which bytes appear
    while (processor.has_more_data()) {
        std::string chunk = processor.read_chunk();
        
        for (char c : chunk) {
            used_bytes.insert(static_cast<unsigned char>(c));
        }
    }
    processor.close();
    
    // Find the first unused byte value (0-255)
    for (int byte_val = 0; byte_val < 256; ++byte_val) {
        if (used_bytes.find(static_cast<unsigned char>(byte_val)) == used_bytes.end()) {
            return byte_val;
        }
    }
    
    // All 256 byte values are used (extremely rare)
    return -1;
}

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

// Process file with forward BWT transform
int bwt_forward_process_file(const char* input_file, const char* output_file, size_t block_size) {
    // Find unique delimiter
    int delimiter_byte = find_unique_char(input_file);
    
    if (delimiter_byte == -1) {
        std::cerr << "Error: Cannot find a unique delimiter (all 256 byte values appear in file)" << std::endl;
        return 1;
    }
    
    char delimiter = static_cast<char>(delimiter_byte);
    
    // Create FileProcessor to handle file I/O
    FileProcessor processor(input_file, output_file, block_size);
    
    if (!processor.is_open()) {
        return 1;
    }
    
    // Write delimiter byte as first byte of output file
    std::string delimiter_str(1, delimiter);
    processor.write_chunk(delimiter_str);
    
    // Process file in chunks
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
    
    // Process the file
    return bwt_forward_process_file(argv[1], argv[2], block_size);
}
#endif // BUILD_TESTS
