#ifndef BWT_HPP
#define BWT_HPP

#include <string>
#include <vector>

// Forward BWT transform
// Takes input string and delimiter, returns BWT transformed string
std::string bwt_forward(const std::string& input, char delimiter = '~');

// Process file with forward BWT transform
// Takes input file path, output file path, and block size
// Returns 0 on success, 1 on error
int bwt_forward_process_file(const char* input_file, const char* output_file, size_t block_size);

#endif // BWT_HPP

