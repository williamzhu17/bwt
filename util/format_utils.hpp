#ifndef FORMAT_UTILS_HPP
#define FORMAT_UTILS_HPP

#include <string>

/**
 * Formatting utility functions for displaying human-readable values.
 */

/**
 * Formats a time value with appropriate units (μs, ms, or s).
 * @param seconds Time value in seconds
 * @return Formatted string with appropriate unit
 */
std::string format_time(double seconds);

/**
 * Formats a file size with appropriate units (B, KB, or MB).
 * @param bytes Size value in bytes
 * @return Formatted string with appropriate unit
 */
std::string format_size(size_t bytes);

#endif // FORMAT_UTILS_HPP

