#include "file_processor.hpp"
#include <iostream>

FileProcessor::FileProcessor(const std::string& input_path, const std::string& output_path, size_t block_size)
    : block_size(block_size), buffer(block_size) {
    // Open input file in binary mode
    input_file.open(input_path, std::ios::binary);
    if (!input_file.is_open()) {
        std::cerr << "Error: Could not open input file " << input_path << std::endl;
    }
    
    // Open output file in binary mode
    output_file.open(output_path, std::ios::binary);
    if (!output_file.is_open()) {
        std::cerr << "Error: Could not open output file " << output_path << std::endl;
        input_file.close();
    }
}

FileProcessor::~FileProcessor() {
    close();
}

bool FileProcessor::is_open() const {
    return input_file.is_open() && output_file.is_open();
}

bool FileProcessor::has_more_data() const {
    return input_file.good();
}

std::string FileProcessor::read_chunk() {
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

void FileProcessor::write_chunk(const std::string& chunk) {
    if (output_file.is_open() && !chunk.empty()) {
        output_file.write(chunk.c_str(), chunk.length());
    }
}

void FileProcessor::close() {
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

