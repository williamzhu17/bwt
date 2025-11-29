#ifndef FILE_PROCESSOR_HPP
#define FILE_PROCESSOR_HPP

#include <string>
#include <fstream>
#include <vector>
#include <mutex>

class FileProcessor {
private:
    std::ifstream input_file;
    std::ofstream output_file;
    size_t block_size;
    std::vector<char> buffer;
    mutable std::mutex mutex_;  // Protects access to input_file, output_file, and buffer

public:
    // Constructor: opens input and output files with specified block size
    FileProcessor(const std::string& input_path, const std::string& output_path, size_t block_size);
    
    // Destructor: closes files if still open
    ~FileProcessor();
    
    // Check if files were opened successfully
    bool is_open() const;
    
    // Check if there's more data to read
    bool has_more_data() const;
    
    // Read a chunk from the input file
    // Returns the chunk as a string, and the actual number of bytes read
    std::string read_chunk();

    // Read a single character from the input file
    // Returns true if successful, false if EOF or error
    bool read_char(char& c);
    
    // Write a chunk to the output file
    void write_chunk(const std::string& chunk);
    
    // Close files explicitly
    void close();
    
    // Get the block size
    size_t get_block_size() const;
};

#endif // FILE_PROCESSOR_HPP
