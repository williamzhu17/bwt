/*
 * Standalone inverse BWT extractor for bzip2's BWT format
 * This reads bzip2's BWT format (marker + origPtr + BWT string) and applies inverse BWT
 */

#include <iostream>
#include <fstream>
#include <vector>
#include <cstring>
#include <algorithm>
#include <map>

// Inverse BWT using origPtr (bzip2's approach)
// BWT string is the last column, origPtr points to the row containing the original string
std::string bzip2_inverse_bwt(const std::vector<unsigned char>& bwt_str, int origPtr) {
    size_t len = bwt_str.size();
    if (len == 0) return "";
    
    // Build occurrence table: Occ(c, i) = number of occurrences of c strictly before position i
    std::vector<size_t> occ_table(len, 0);
    std::map<unsigned char, size_t> occ_before;
    
    for (size_t i = 0; i < len; ++i) {
        unsigned char ch = bwt_str[i];
        size_t occ = occ_before.count(ch) ? occ_before[ch] : 0;
        occ_table[i] = occ;
        occ_before[ch] = occ + 1;
    }
    
    // C(c): index of the first occurrence of c in the sorted first column
    std::map<unsigned char, size_t> first_occurrence;
    size_t total = 0;
    for (const auto& entry : occ_before) {
        first_occurrence[entry.first] = total;
        total += entry.second;
    }
    
    // Reconstruct original string by following LF mapping starting from origPtr
    std::vector<char> result;
    size_t row = origPtr;
    
    for (size_t i = 0; i < len; ++i) {
        unsigned char ch = bwt_str[row];
        result.push_back(static_cast<char>(ch));
        row = first_occurrence[ch] + occ_table[row];
    }
    
    // Reverse to get original string (we built it backwards)
    std::reverse(result.begin(), result.end());
    return std::string(result.begin(), result.end());
}

// Process file with inverse BWT (reads bzip2 format)
int bzip2_inverse_bwt_process_file(const char* input_file, const char* output_file, 
                                   size_t block_size) {
    std::ifstream in(input_file, std::ios::binary);
    if (!in.is_open()) {
        std::cerr << "Error: Cannot open input file: " << input_file << std::endl;
        return 1;
    }
    
    std::ofstream out(output_file, std::ios::binary);
    if (!out.is_open()) {
        std::cerr << "Error: Cannot open output file: " << output_file << std::endl;
        return 1;
    }
    
    // Process blocks
    while (in.good()) {
        // Read marker byte (should be 0xFF)
        unsigned char marker;
        in.read((char*)&marker, 1);
        if (in.gcount() == 0) break;
        
        if (marker != 0xFF) {
            std::cerr << "Error: Invalid marker byte: 0x" << std::hex << (int)marker << std::endl;
            return 1;
        }
        
        // Read origPtr (3 bytes, big-endian)
        unsigned char origPtr_bytes[3];
        in.read((char*)origPtr_bytes, 3);
        if (in.gcount() != 3) {
            std::cerr << "Error: Failed to read origPtr" << std::endl;
            return 1;
        }
        
        int origPtr = (origPtr_bytes[0] << 16) | (origPtr_bytes[1] << 8) | origPtr_bytes[2];
        
        // Read BWT string (size should match block_size)
        std::vector<unsigned char> bwt_output(block_size);
        in.read((char*)bwt_output.data(), block_size);
        size_t bytes_read = in.gcount();
        
        if (bytes_read == 0) break;
        
        // Resize if we read less than block_size (last block)
        if (bytes_read < block_size) {
            bwt_output.resize(bytes_read);
        }
        
        // Validate origPtr
        if (origPtr < 0 || origPtr >= (int)bytes_read) {
            std::cerr << "Error: Invalid origPtr: " << origPtr << " (block size: " << bytes_read << ")" << std::endl;
            return 1;
        }
        
        // Apply inverse BWT
        std::string result = bzip2_inverse_bwt(bwt_output, origPtr);
        
        // Write result
        out.write(result.c_str(), result.size());
    }
    
    return 0;
}

int main(int argc, char* argv[]) {
    if (argc < 3 || argc > 4) {
        std::cerr << "Usage: " << argv[0] << " <input_file> <output_file> [block_size]" << std::endl;
        std::cerr << "  block_size: size of each block in bytes (default: 65536)" << std::endl;
        return 1;
    }
    
    size_t block_size = 65536;  // 64KB default
    if (argc == 4) {
        block_size = std::stoul(argv[3]);
        if (block_size == 0) {
            std::cerr << "Error: Block size must be greater than 0" << std::endl;
            return 1;
        }
    }
    
    return bzip2_inverse_bwt_process_file(argv[1], argv[2], block_size);
}

