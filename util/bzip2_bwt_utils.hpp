#ifndef BZIP2_BWT_UTILS_HPP
#define BZIP2_BWT_UTILS_HPP

#include <string>
#include <vector>
#include <fstream>
#include <map>

// Include bzip2 headers for EState definition
// EState is a typedef defined in bzlib_private.h
// We need the complete type in the header for function signatures
extern "C" {
#include "../bzip2/bzlib_private.h"
}

/**
 * Utility class for bzip2 BWT operations
 * Encapsulates bzip2's BWT implementation without compression
 */
class Bzip2BWTProcessor {
public:
    /**
     * Convert arbitrary block size to bzip2's internal block size parameter
     * Supports any block size; converts to bzip2's internal format (1-9 multiplier of 100KB)
     * @param block_size Desired block size in bytes (arbitrary value)
     * @return bzip2 internal block size parameter (1-9)
     */
    static int calculate_bzip2_internal_block_size(size_t block_size);
    
    /**
     * Initialize EState for BWT only (no compression)
     * @param bzip2_block_param bzip2 internal block size parameter (1-9)
     * @param verbosity Verbosity level (0 = silent)
     * @param workFactor Work factor for sorting (default 30)
     * @return Pointer to initialized EState, or nullptr on failure
     */
    static EState* init_bwt_state(int bzip2_block_param, int verbosity, int workFactor);
    
    /**
     * Free EState and all allocated resources
     * @param s Pointer to EState to free
     */
    static void free_bwt_state(EState* s);
    
    /**
     * Extract BWT output from sorted block
     * For each position i in sorted order, BWT[i] is the character before 
     * the suffix starting at ptr[i]
     * @param s EState containing sorted block
     * @param bwt_output Output vector to store BWT result
     */
    static void extract_bwt_output(EState* s, std::vector<unsigned char>& bwt_output);
    
    /**
     * Process a single block of data with BWT
     * @param s EState for BWT processing
     * @param data Input data block
     * @param size Size of input data block
     * @param bwt_output Output vector for BWT result
     * @param origPtr Output parameter for original pointer
     * @return true on success, false on failure
     */
    static bool process_block(EState* s, const unsigned char* data, int size,
                              std::vector<unsigned char>& bwt_output, int& origPtr);
    
    /**
     * Write bzip2 format block to output stream
     * Format: marker byte (0xFF) + origPtr (3 bytes, big-endian) + BWT output
     * @param out Output stream
     * @param origPtr Original pointer value
     * @param bwt_output BWT transform result
     * @return true on success, false on failure
     */
    static bool write_bzip2_block(std::ofstream& out, int origPtr,
                                   const std::vector<unsigned char>& bwt_output);
    
    /**
     * Read bzip2 format block from input stream
     * @param in Input stream
     * @param block_size Expected block size
     * @param origPtr Output parameter for original pointer
     * @param bwt_output Output vector for BWT string
     * @return true on success, false on failure or EOF
     */
    static bool read_bzip2_block(std::ifstream& in, size_t block_size,
                                  int& origPtr, std::vector<unsigned char>& bwt_output);
    
    /**
     * Apply inverse BWT using origPtr (bzip2's approach)
     * BWT string is the last column, origPtr points to the row containing the original string
     * @param bwt_str BWT transformed string
     * @param origPtr Original pointer indicating starting row
     * @return Reconstructed original string
     */
    static std::string inverse_bwt(const std::vector<unsigned char>& bwt_str, int origPtr);
    
    /**
     * Process file with forward BWT (extracts only BWT transform without compression)
     * @param input_file Input file path
     * @param output_file Output file path
     * @param block_size Size of each block in bytes
     * @return 0 on success, 1 on failure
     */
    static int process_file_forward(const char* input_file, const char* output_file,
                                    size_t block_size);
    
    /**
     * Process file with inverse BWT (reads bzip2 format)
     * @param input_file Input file path (bzip2 BWT format)
     * @param output_file Output file path
     * @param block_size Size of each block in bytes
     * @return 0 on success, 1 on failure
     */
    static int process_file_inverse(const char* input_file, const char* output_file,
                                    size_t block_size);

private:
    /**
     * Build occurrence table for inverse BWT
     * Occ(c, i) = number of occurrences of c strictly before position i
     * @param bwt_str BWT transformed string
     * @param occ_table Output occurrence table
     * @param occ_before Output map of character occurrences
     */
    static void build_occurrence_table(const std::vector<unsigned char>& bwt_str,
                                       std::vector<size_t>& occ_table,
                                       std::map<unsigned char, size_t>& occ_before);
    
    /**
     * Build first occurrence table for inverse BWT
     * C(c) = index of the first occurrence of c in the sorted first column
     * @param occ_before Map of character occurrences
     * @return Map of first occurrence indices
     */
    static std::map<unsigned char, size_t> build_first_occurrence(
        const std::map<unsigned char, size_t>& occ_before);
    
    /**
     * Initialize inUse array in EState based on input data
     * @param s EState to initialize
     * @param data Input data
     * @param size Size of input data
     */
    static void initialize_in_use_array(EState* s, const unsigned char* data, int size);
    
    /**
     * Validate block size against EState maximum
     * @param s EState containing maximum block size
     * @param size Block size to validate
     * @return true if valid, false if exceeds maximum
     */
    static bool validate_block_size(EState* s, int size);
};

#endif // BZIP2_BWT_UTILS_HPP

