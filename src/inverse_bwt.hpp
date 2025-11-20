#ifndef INVERSE_BWT_HPP
#define INVERSE_BWT_HPP

#include <string>

// Inverse BWT transform
// Takes BWT string and delimiter, returns original string
std::string bwt_inverse(const std::string& bwt_str, char delimiter = '~');

// Process file with inverse BWT transform
// Takes input file path, output file path, and block size
// Returns 0 on success, 1 on error
int bwt_inverse_process_file(const char* input_file, const char* output_file, size_t block_size);

#endif // INVERSE_BWT_HPP

