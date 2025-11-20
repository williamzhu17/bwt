#include "bwt.hpp"
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
    std::vector<char> buffer(block_size);
    char delimiter = '~';
    
    while (file.good()) {
        // Read a chunk
        file.read(buffer.data(), block_size);
        size_t bytes_read = file.gcount();
        
        if (bytes_read == 0) {
            break;
        }
        
        std::string chunk(buffer.data(), bytes_read);
        
        // If the last chunk is smaller than block_size + 1, write it straight up without transforming
        std::string result = bwt_forward(chunk, delimiter);
        out_file.write(result.c_str(), result.length());
    }
    
    file.close();
    out_file.close();
    return 0;
}
