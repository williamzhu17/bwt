#ifndef INVERSE_BWT_HPP
#define INVERSE_BWT_HPP

#include <string>

// Inverse BWT transform
// Takes BWT string and delimiter, returns original string
std::string bwt_inverse(const std::string& bwt_str, char delimiter = '~');

#endif // INVERSE_BWT_HPP

