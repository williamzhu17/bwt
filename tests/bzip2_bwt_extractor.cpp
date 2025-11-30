/*
 * Standalone BWT extractor using bzip2's BWT implementation
 * This extracts only the BWT transform without compression
 */

#include <iostream>
#include <fstream>
#include <vector>
#include <cstring>

// Include bzip2 headers
extern "C" {
#include "../bzip2/bzlib_private.h"
}

// Forward declarations from bzip2 (will be linked from bzlib.c, blocksort.c, etc.)
extern "C" {
    void BZ2_blockSort(EState* s);
    void BZ2_bz__AssertH__fail(int errcode);
    extern UInt32 BZ2_crc32Table[256];
}

// Initialize EState for BWT only (no compression)
static EState* init_bwt_state(int blockSize100k, int verbosity, int workFactor) {
    EState* s = (EState*)malloc(sizeof(EState));
    if (!s) return NULL;
    
    memset(s, 0, sizeof(EState));
    
    Int32 n = 100000 * blockSize100k;
    s->arr1 = (UInt32*)malloc(n * sizeof(UInt32));
    s->arr2 = (UInt32*)malloc((n + BZ_N_OVERSHOOT) * sizeof(UInt32));
    s->ftab = (UInt32*)malloc(65537 * sizeof(UInt32));
    
    if (!s->arr1 || !s->arr2 || !s->ftab) {
        if (s->arr1) free(s->arr1);
        if (s->arr2) free(s->arr2);
        if (s->ftab) free(s->ftab);
        free(s);
        return NULL;
    }
    
    s->blockSize100k = blockSize100k;
    s->nblockMAX = 100000 * blockSize100k - 19;
    s->verbosity = verbosity;
    s->workFactor = workFactor;
    
    s->block = (UChar*)s->arr2;
    s->ptr = (UInt32*)s->arr1;
    
    return s;
}

// Free EState
static void free_bwt_state(EState* s) {
    if (!s) return;
    if (s->arr1) free(s->arr1);
    if (s->arr2) free(s->arr2);
    if (s->ftab) free(s->ftab);
    free(s);
}

// Extract BWT output from sorted block
static void extract_bwt_output(EState* s, std::vector<UChar>& bwt_output) {
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

// Process a single block
static bool process_block(EState* s, const UChar* data, Int32 size, 
                          std::vector<UChar>& bwt_output, Int32& origPtr) {
    if (size > s->nblockMAX) {
        std::cerr << "Error: Block size " << size << " exceeds maximum " 
                  << s->nblockMAX << std::endl;
        return false;
    }
    
    // Copy data into block
    s->nblock = size;
    memcpy(s->block, data, size);
    
    // Initialize inUse array
    memset(s->inUse, 0, 256);
    for (Int32 i = 0; i < size; i++) {
        s->inUse[data[i]] = True;
    }
    
    // Perform BWT
    BZ2_blockSort(s);
    
    // Extract BWT output
    extract_bwt_output(s, bwt_output);
    
    // Get original pointer
    origPtr = s->origPtr;
    
    return true;
}

// Process file with BWT (similar to your implementation)
int bzip2_bwt_process_file(const char* input_file, const char* output_file, 
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
    
    // Convert block_size to bzip2's blockSize100k format
    // bzip2 uses blocks of 100k * blockSize100k bytes
    // nblockMAX = 100000 * blockSize100k - 19
    // We need to find the minimum blockSize100k such that block_size <= nblockMAX
    // block_size <= 100000 * blockSize100k - 19
    // block_size + 19 <= 100000 * blockSize100k
    // blockSize100k >= ceil((block_size + 19) / 100000)
    int blockSize100k = ((block_size + 19) + 99999) / 100000;  // Ceiling division
    if (blockSize100k < 1) blockSize100k = 1;
    if (blockSize100k > 9) blockSize100k = 9;  // bzip2 max is 9
    
    // Initialize BWT state
    EState* s = init_bwt_state(blockSize100k, 0, 30);
    if (!s) {
        std::cerr << "Error: Failed to initialize BWT state" << std::endl;
        return 1;
    }
    
    // Read and process blocks
    std::vector<UChar> buffer(block_size);
    std::vector<UChar> bwt_output;
    
    while (in.good()) {
        // Read a block
        in.read((char*)buffer.data(), block_size);
        size_t bytes_read = in.gcount();
        
        if (bytes_read == 0) break;
        
        // Process block
        Int32 origPtr;
        if (!process_block(s, buffer.data(), bytes_read, bwt_output, origPtr)) {
            free_bwt_state(s);
            return 1;
        }
        
        // Write bzip2 format: marker byte (0xFF) + origPtr (3 bytes) + BWT output
        UChar marker = 0xFF;
        out.write((char*)&marker, 1);
        
        // Write origPtr (3 bytes, big-endian, matching bzip2's format)
        UChar origPtr_bytes[3];
        origPtr_bytes[0] = (origPtr >> 16) & 0xFF;
        origPtr_bytes[1] = (origPtr >> 8) & 0xFF;
        origPtr_bytes[2] = origPtr & 0xFF;
        out.write((char*)origPtr_bytes, 3);
        
        // Write BWT output (this is the actual BWT transform result)
        out.write((char*)bwt_output.data(), bwt_output.size());
    }
    
    free_bwt_state(s);
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
    
    return bzip2_bwt_process_file(argv[1], argv[2], block_size);
}
