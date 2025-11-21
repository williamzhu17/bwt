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

// Build suffix array using prefix-doubling algorithm
std::vector<size_t> build_suffix_array(const std::string& input) {
    size_t n = input.length();

    std::vector<size_t> suffix_array(n);
    std::vector<size_t> rank(n);
    std::vector<size_t> temp(n);

    // Initialize ranks based on first character
    for (size_t i = 0; i < n; i++) {
        suffix_array[i] = i;
        rank[i] = static_cast<unsigned char>(input[i]);
    }

    // Sort suffixes by first k characters, then 2k, etc.
    for (size_t k = 1; k < n; k *= 2) {
        // Compare based on current rank pairs
        auto cmp = [&](size_t a, size_t b) {
            if (rank[a] != rank[b]) return rank[a] < rank[b];
            
            // Compare next block (k steps ahead)
            bool a_has_next = (a + k < n);
            bool b_has_next = (b + k < n);
            
            if (a_has_next && b_has_next) {
                return rank[a + k] < rank[b + k];
            }
            
            // If one doesn't exist (past end), it is considered smaller
            // If a is past end, a < b. If b is past end, b < a (so a > b)
            return !a_has_next && b_has_next;
        };

        std::sort(suffix_array.begin(), suffix_array.end(), cmp);

        // Recompute ranks based on sorted order
        temp[suffix_array[0]] = 0;
        for (size_t i = 1; i < n; i++) {
            temp[suffix_array[i]] = temp[suffix_array[i - 1]] + (cmp(suffix_array[i - 1], suffix_array[i]) ? 1 : 0);
        }

        rank = temp;

        // If max rank is n-1, all suffixes are distinct
        if (rank[suffix_array[n - 1]] == n - 1) {
            break;
        }
    }

    return suffix_array;
}

// Forward BWT transform
std::string bwt_forward(const std::string& input, char delimiter) {
    std::string s = input + delimiter;
    size_t n = s.length();
    
    // Build suffix array for input+delimiter
    std::vector<size_t> sa = build_suffix_array(s);
    
    // Construct BWT string
    std::string bwt_str;
    bwt_str.resize(n);
    
    for (size_t i = 0; i < n; i++) {
        // BWT[i] is the character preceding the i-th sorted suffix
        if (sa[i] == 0) {
            bwt_str[i] = s[n - 1];
        } else {
            bwt_str[i] = s[sa[i] - 1];
        }
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
