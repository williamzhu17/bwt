/*
 * Utility functions for bzip2 BWT operations
 * Encapsulates bzip2's BWT implementation without compression
 */

#include "bzip2_bwt_utils.hpp"
#include <iostream>
#include <fstream>
#include <vector>
#include <cstring>
#include <algorithm>
#include <map>
#include <iomanip>

// Forward declarations from bzip2 (will be linked from bzlib.c, blocksort.c, etc.)
extern "C" {
    void BZ2_blockSort(EState* s);
    void BZ2_bz__AssertH__fail(int errcode);
    extern UInt32 BZ2_crc32Table[256];
}

int Bzip2BWTProcessor::calculate_bzip2_internal_block_size(size_t block_size) {
    // Convert arbitrary block size to bzip2's internal format
    // bzip2 internally uses blocks of 100k * bzip2_block_param bytes
    // nblockMAX = 100000 * bzip2_block_param - 19
    // We find the minimum bzip2 internal parameter such that block_size <= nblockMAX
    // block_size <= 100000 * bzip2_block_param - 19
    // block_size + 19 <= 100000 * bzip2_block_param
    // bzip2_block_param >= ceil((block_size + 19) / 100000)
    int bzip2_block_param = ((block_size + 19) + 99999) / 100000;  // Ceiling division
    if (bzip2_block_param < 1) bzip2_block_param = 1;
    if (bzip2_block_param > 9) bzip2_block_param = 9;  // bzip2 max is 9
    return bzip2_block_param;
}

EState* Bzip2BWTProcessor::init_bwt_state(int bzip2_block_param, int verbosity, int workFactor) {
    EState* s = (EState*)malloc(sizeof(EState));
    if (!s) return nullptr;
    
    memset(s, 0, sizeof(EState));
    
    Int32 n = 100000 * bzip2_block_param;
    s->arr1 = (UInt32*)malloc(n * sizeof(UInt32));
    s->arr2 = (UInt32*)malloc((n + BZ_N_OVERSHOOT) * sizeof(UInt32));
    s->ftab = (UInt32*)malloc(65537 * sizeof(UInt32));
    
    if (!s->arr1 || !s->arr2 || !s->ftab) {
        if (s->arr1) free(s->arr1);
        if (s->arr2) free(s->arr2);
        if (s->ftab) free(s->ftab);
        free(s);
        return nullptr;
    }
    
    // Note: blockSize100k is bzip2's internal field name (part of their API)
    s->blockSize100k = bzip2_block_param;
    s->nblockMAX = 100000 * bzip2_block_param - 19;
    s->verbosity = verbosity;
    s->workFactor = workFactor;
    
    s->block = (UChar*)s->arr2;
    s->ptr = (UInt32*)s->arr1;
    
    return s;
}

void Bzip2BWTProcessor::free_bwt_state(EState* s) {
    if (!s) return;
    if (s->arr1) free(s->arr1);
    if (s->arr2) free(s->arr2);
    if (s->ftab) free(s->ftab);
    free(s);
}

void Bzip2BWTProcessor::extract_bwt_output(EState* s, std::vector<unsigned char>& bwt_output) {
    UInt32* ptr = s->ptr;
    UChar* block = s->block;
    Int32 nblock = s->nblock;
    
    // Reserve space for BWT output
    bwt_output.resize(nblock);
    
    // Extract BWT: for each position i in sorted order,
    // BWT[i] is the character before the suffix starting at ptr[i]
    for (Int32 i = 0; i < nblock; i++) {
        Int32 pos = ptr[i];
        if (pos == 0) {
            // Wrap around: character before position 0 is at end
            bwt_output[i] = block[nblock - 1];
        } else {
            bwt_output[i] = block[pos - 1];
        }
    }
}

bool Bzip2BWTProcessor::validate_block_size(EState* s, int size) {
    if (size > s->nblockMAX) {
        std::cerr << "Error: Block size " << size << " exceeds maximum " 
                  << s->nblockMAX << std::endl;
        return false;
    }
    return true;
}

void Bzip2BWTProcessor::initialize_in_use_array(EState* s, const unsigned char* data, int size) {
    memset(s->inUse, 0, 256);
    for (int i = 0; i < size; i++) {
        s->inUse[data[i]] = True;
    }
}

bool Bzip2BWTProcessor::process_block(EState* s, const unsigned char* data, int size,
                                      std::vector<unsigned char>& bwt_output, int& origPtr) {
    if (!validate_block_size(s, size)) {
        return false;
    }
    
    // Copy data into block
    s->nblock = size;
    memcpy(s->block, data, size);
    
    // Initialize inUse array
    initialize_in_use_array(s, data, size);
    
    // Perform BWT
    BZ2_blockSort(s);
    
    // Extract BWT output
    extract_bwt_output(s, bwt_output);
    
    // Get original pointer
    origPtr = s->origPtr;
    
    return true;
}

bool Bzip2BWTProcessor::write_bzip2_block(std::ofstream& out, int origPtr,
                                           const std::vector<unsigned char>& bwt_output) {
    // Write marker byte (0xFF)
    unsigned char marker = 0xFF;
    out.write((char*)&marker, 1);
    if (!out.good()) return false;
    
    // Write origPtr (3 bytes, big-endian, matching bzip2's format)
    unsigned char origPtr_bytes[3];
    origPtr_bytes[0] = (origPtr >> 16) & 0xFF;
    origPtr_bytes[1] = (origPtr >> 8) & 0xFF;
    origPtr_bytes[2] = origPtr & 0xFF;
    out.write((char*)origPtr_bytes, 3);
    if (!out.good()) return false;
    
    // Write BWT output (this is the actual BWT transform result)
    out.write((char*)bwt_output.data(), bwt_output.size());
    return out.good();
}

bool Bzip2BWTProcessor::read_bzip2_block(std::ifstream& in, size_t block_size,
                                          int& origPtr, std::vector<unsigned char>& bwt_output) {
    // Read marker byte (should be 0xFF)
    unsigned char marker;
    in.read((char*)&marker, 1);
    if (in.gcount() == 0) return false;
    
    if (marker != 0xFF) {
        std::cerr << "Error: Invalid marker byte: 0x" << std::hex << (int)marker << std::endl;
        return false;
    }
    
    // Read origPtr (3 bytes, big-endian)
    unsigned char origPtr_bytes[3];
    in.read((char*)origPtr_bytes, 3);
    if (in.gcount() != 3) {
        std::cerr << "Error: Failed to read origPtr" << std::endl;
        return false;
    }
    
    origPtr = (origPtr_bytes[0] << 16) | (origPtr_bytes[1] << 8) | origPtr_bytes[2];
    
    // Read BWT string (size should match block_size)
    bwt_output.resize(block_size);
    in.read((char*)bwt_output.data(), block_size);
    size_t bytes_read = in.gcount();
    
    if (bytes_read == 0) return false;
    
    // Resize if we read less than block_size (last block)
    if (bytes_read < block_size) {
        bwt_output.resize(bytes_read);
    }
    
    // Validate origPtr
    if (origPtr < 0 || origPtr >= (int)bytes_read) {
        std::cerr << "Error: Invalid origPtr: " << origPtr << " (block size: " << bytes_read << ")" << std::endl;
        return false;
    }
    
    return true;
}

void Bzip2BWTProcessor::build_occurrence_table(const std::vector<unsigned char>& bwt_str,
                                                std::vector<size_t>& occ_table,
                                                std::map<unsigned char, size_t>& occ_before) {
    size_t len = bwt_str.size();
    occ_table.resize(len, 0);
    occ_before.clear();
    
    // Build occurrence table: Occ(c, i) = number of occurrences of c strictly before position i
    for (size_t i = 0; i < len; ++i) {
        unsigned char ch = bwt_str[i];
        size_t occ = occ_before.count(ch) ? occ_before[ch] : 0;
        occ_table[i] = occ;
        occ_before[ch] = occ + 1;
    }
}

std::map<unsigned char, size_t> Bzip2BWTProcessor::build_first_occurrence(
    const std::map<unsigned char, size_t>& occ_before) {
    // C(c): index of the first occurrence of c in the sorted first column
    std::map<unsigned char, size_t> first_occurrence;
    size_t total = 0;
    for (const auto& entry : occ_before) {
        first_occurrence[entry.first] = total;
        total += entry.second;
    }
    return first_occurrence;
}

std::string Bzip2BWTProcessor::inverse_bwt(const std::vector<unsigned char>& bwt_str, int origPtr) {
    size_t len = bwt_str.size();
    if (len == 0) return "";
    
    // Build occurrence table
    std::vector<size_t> occ_table;
    std::map<unsigned char, size_t> occ_before;
    build_occurrence_table(bwt_str, occ_table, occ_before);
    
    // Build first occurrence table
    std::map<unsigned char, size_t> first_occurrence = build_first_occurrence(occ_before);
    
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

int Bzip2BWTProcessor::process_file_forward(const char* input_file, const char* output_file,
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
    
    // Convert arbitrary block size to bzip2's internal format
    int bzip2_block_param = calculate_bzip2_internal_block_size(block_size);
    
    // Initialize BWT state
    EState* s = init_bwt_state(bzip2_block_param, 0, 30);
    if (!s) {
        std::cerr << "Error: Failed to initialize BWT state" << std::endl;
        return 1;
    }
    
    // Read and process blocks
    std::vector<unsigned char> buffer(block_size);
    std::vector<unsigned char> bwt_output;
    
    while (in.good()) {
        // Read a block
        in.read((char*)buffer.data(), block_size);
        size_t bytes_read = in.gcount();
        
        if (bytes_read == 0) break;
        
        // Process block
        int origPtr;
        if (!process_block(s, buffer.data(), bytes_read, bwt_output, origPtr)) {
            free_bwt_state(s);
            return 1;
        }
        
        // Write bzip2 format block
        if (!write_bzip2_block(out, origPtr, bwt_output)) {
            free_bwt_state(s);
            return 1;
        }
    }
    
    free_bwt_state(s);
    return 0;
}

int Bzip2BWTProcessor::process_file_inverse(const char* input_file, const char* output_file,
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
        int origPtr;
        std::vector<unsigned char> bwt_output;
        
        if (!read_bzip2_block(in, block_size, origPtr, bwt_output)) {
            break;  // EOF or error
        }
        
        // Apply inverse BWT
        std::string result = inverse_bwt(bwt_output, origPtr);
        
        // Write result
        out.write(result.c_str(), result.size());
        if (!out.good()) {
            return 1;
        }
    }
    
    return 0;
}

