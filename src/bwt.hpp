#ifndef BWT_HPP
#define BWT_HPP

#include <string>
#include <vector>

// Forward BWT transform
// Takes input string and delimiter, returns BWT transformed string
std::string bwt_forward(const std::string& input, char delimiter = '~');

#endif // BWT_HPP

