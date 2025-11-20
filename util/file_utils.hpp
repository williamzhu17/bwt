#ifndef FILE_UTILS_HPP
#define FILE_UTILS_HPP

#include <string>
#include <vector>

/**
 * File system utility functions for BWT testing and performance benchmarking.
 */

/**
 * Creates a directory if it doesn't already exist.
 * @param dir_path Path to the directory to create
 * @return true if directory exists or was created successfully, false otherwise
 */
bool create_directory(const std::string& dir_path);

/**
 * Checks if a file exists.
 * @param filename Path to the file to check
 * @return true if file exists, false otherwise
 */
bool file_exists(const std::string& filename);

/**
 * Checks if a directory exists.
 * @param dir_path Path to the directory to check
 * @return true if directory exists, false otherwise
 */
bool directory_exists(const std::string& dir_path);

/**
 * Gets the size of a file in bytes.
 * @param filename Path to the file
 * @return File size in bytes, or 0 if file doesn't exist or cannot be read
 */
size_t get_file_size(const std::string& filename);

/**
 * Lists all regular files in a directory (non-recursive).
 * @param dir_path Path to the directory to list
 * @return Vector of filenames (not full paths) found in the directory
 */
std::vector<std::string> list_files_in_directory(const std::string& dir_path);

/**
 * Compares two files byte by byte to check if they are identical.
 * @param file1 Path to the first file
 * @param file2 Path to the second file
 * @return true if files are identical, false otherwise
 */
bool files_are_identical(const std::string& file1, const std::string& file2);

#endif // FILE_UTILS_HPP

