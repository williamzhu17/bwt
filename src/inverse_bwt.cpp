#include "inverse_bwt.hpp"
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


int main(int argc, char* argv[]) {
    // Check for command line arguments
    if (argc < 3 || argc > 4) {
        std::cerr << "Usage: " << argv[0] << " <input_file> <output_file> [block_size]" << std::endl;
        std::cerr << "  block_size: size of each block in bytes (default: 65536)" << std::endl;
        return 1;
    }
    
    // Parse block size (default 32KB)
    size_t block_size = 32768;
    if (argc == 4) {
        block_size = std::stoul(argv[3]);
        if (block_size == 0) {
            std::cerr << "Error: Block size must be greater than 0" << std::endl;
            return 1;
        }
    }
    
    // Open input file
    std::ifstream file(argv[1], std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "Error: Could not open input file " << argv[1] << std::endl;
        return 1;
    }
    
    // Open output file
    std::ofstream out_file(argv[2], std::ios::binary);
    if (!out_file.is_open()) {
        std::cerr << "Error: Could not open output file " << argv[2] << std::endl;
        file.close();
        return 1;
    }
    
    // Process file in chunks
    // Note: Forward BWT outputs chunks of size (input_size + 1) due to delimiter
    // So we need to read chunks of size (block_size + 1) to match
    size_t bwt_chunk_size = block_size + 1;
    std::vector<char> buffer(bwt_chunk_size);
    char delimiter = '~';
    
    while (file.good()) {
        // Read a chunk (size block_size + 1 to account for delimiter)
        file.read(buffer.data(), bwt_chunk_size);
        size_t bytes_read = file.gcount();
        
        if (bytes_read == 0) {
            break;
        }
        
        // Convert chunk to string (only the bytes we actually read)
        std::string chunk(buffer.data(), bytes_read);
        
        // If the last chunk is smaller than block_size + 1, write it straight up without inverting
        if (bytes_read < bwt_chunk_size) {
            out_file.write(chunk.c_str(), chunk.length());
        } else {
            // Apply inverse transform to this chunk and write
            std::string result = bwt_inverse(chunk, delimiter);
            out_file.write(result.c_str(), result.length());
        }
    }
    
    file.close();
    out_file.close();
    return 0;
}
