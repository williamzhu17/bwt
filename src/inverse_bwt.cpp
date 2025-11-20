#include "inverse_bwt.hpp"
#include "file_processor.hpp"
#include <iostream>
#include <vector>
#include <map>
#include <unordered_map>
#include <algorithm>
#include <string>
#include <fstream>
#include <cstring>

// Inverse BWT transform
std::string bwt_inverse(const std::string& bwt_str, char delimiter) {
    size_t len = bwt_str.length();
    std::string last_column = bwt_str;

    std::vector<size_t> occ_table(len, 0);
    std::unordered_map<unsigned char, size_t> occ_before;
    size_t delimiter_row = std::string::npos;

    // Count characters and record Occ(c, i) = number of occurrences of `c`
    // strictly before position i in the last column.
    for (size_t i = 0; i < len; ++i) {
        unsigned char ch = static_cast<unsigned char>(last_column[i]);
        if (last_column[i] == delimiter) {
            delimiter_row = i;
        }

        size_t occ = occ_before.count(ch) ? occ_before[ch] : 0;
        occ_table[i] = occ;
        occ_before[ch] = occ + 1;
    }

    // C(c): index of the first occurrence of `c` in the sorted first column.
    std::map<unsigned char, size_t> first_occurrence;
    for (const auto& entry : occ_before) {
        first_occurrence[entry.first] = 0;
    }

    size_t total = 0;
    for (auto& entry : first_occurrence) {
        unsigned char ch = entry.first;
        entry.second = total;
        total += occ_before[ch];
    }

    // Follow Langmead's pseudocode: rebuild string by iteratively applying LF.
    std::vector<char> result;
    size_t row = delimiter_row;

    while (1) {
        unsigned char next_char = static_cast<unsigned char>(bwt_str[row]);
        row = first_occurrence[next_char] + occ_table[row];
        unsigned char ch = static_cast<unsigned char>(bwt_str[row]);
        if (ch == static_cast<unsigned char>(delimiter)) {
            break;
        }
        result.push_back(static_cast<char>(ch));
    }
    // Reverse the result and join as a string
    std::reverse(result.begin(), result.end());
    return std::string(result.begin(), result.end());    
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
    char delimiter = '$';
    
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
