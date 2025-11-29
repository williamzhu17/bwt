#include "file_processor.hpp"
#include <iostream>

FileProcessor::FileProcessor(const std::string& input_path, const std::string& output_path, size_t block_size)
    : block_size(block_size), buffer(block_size) {
    // Open input file in binary mode
    input_file.open(input_path, std::ios::binary);
    if (!input_file.is_open()) {
        std::cerr << "Error: Could not open input file " << input_path << std::endl;
    }
    
    // Open output file in binary mode if path is provided
    if (!output_path.empty()) {
        output_file.open(output_path, std::ios::binary);
        if (!output_file.is_open()) {
            std::cerr << "Error: Could not open output file " << output_path << std::endl;
            input_file.close();
        }
    }
}

FileProcessor::~FileProcessor() {
    close();
}

bool FileProcessor::is_open() const {
    std::lock_guard<std::mutex> lock(mutex_);
    // If input is open, and (either output is not needed (closed but no error?) or output is open)
    // If we requested output and it failed, input_file would be closed in constructor.
    return input_file.is_open();
}

bool FileProcessor::has_more_data() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return input_file.good();
}

std::string FileProcessor::read_chunk() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!input_file.good()) {
        return "";
    }
    
    // Read a chunk
    input_file.read(buffer.data(), block_size);
    size_t bytes_read = input_file.gcount();
    
    if (bytes_read == 0) {
        return "";
    }
    
    return std::string(buffer.data(), bytes_read);
}

bool FileProcessor::read_char(char& c) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!input_file.good()) {
        return false;
    }
    input_file.read(&c, 1);
    return input_file.gcount() == 1;
}

void FileProcessor::write_chunk(const std::string& chunk) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (output_file.is_open() && !chunk.empty()) {
        output_file.write(chunk.c_str(), chunk.length());
    }
}

void FileProcessor::close() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (input_file.is_open()) {
        input_file.close();
    }
    if (output_file.is_open()) {
        output_file.close();
    }
}

size_t FileProcessor::get_block_size() const {
    return block_size;
}
