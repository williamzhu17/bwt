/*
 * Standalone BWT extractor using bzip2's BWT implementation
 * This extracts only the BWT transform without compression
 */

#include <iostream>
#include "../util/bzip2_bwt_utils.hpp"

/**
 * Parse block size from command line argument
 * @param argc Argument count
 * @param argv Argument vector
 * @return Block size in bytes, or 0 on error
 */
static size_t parse_block_size(int argc, char* argv[]) {
    const size_t DEFAULT_BLOCK_SIZE = 65536;  // 64KB default
    
    if (argc == 4) {
        size_t block_size = std::stoul(argv[3]);
        if (block_size == 0) {
            std::cerr << "Error: Block size must be greater than 0" << std::endl;
            return 0;
        }
        return block_size;
    }
    
    return DEFAULT_BLOCK_SIZE;
}

int main(int argc, char* argv[]) {
    if (argc < 3 || argc > 4) {
        std::cerr << "Usage: " << argv[0] << " <input_file> <output_file> [block_size]" << std::endl;
        std::cerr << "  block_size: size of each block in bytes (default: 65536)" << std::endl;
        return 1;
    }
    
    size_t block_size = parse_block_size(argc, argv);
    if (block_size == 0) {
        return 1;
    }
    
    return Bzip2BWTProcessor::process_file_forward(argv[1], argv[2], block_size);
}
