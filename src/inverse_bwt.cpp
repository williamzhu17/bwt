#include "inverse_bwt.hpp"
#include "file_processor.hpp"
#include <iostream>
#include <vector>
#include <map>
#include <unordered_map>
#include <algorithm>
#include <string>
#include <fstream>
#include <cstring>
#include <thread>
#include <atomic>

#include "../util/blocking_queue.hpp"
#include "../util/reorder_buffer.hpp"

// Inverse BWT transform
std::string bwt_inverse(const std::string& bwt_str, char delimiter) {
    size_t len = bwt_str.length();
    std::string last_column = bwt_str;

    std::vector<size_t> occ_table(len, 0);
    std::unordered_map<unsigned char, size_t> occ_before;
    size_t delimiter_row = std::string::npos;

    // Count characters and record Occ(c, i) = number of occurrences of `c`
    // strictly before position i in the last column.
    for (size_t i = 0; i < len; ++i) {
        unsigned char ch = static_cast<unsigned char>(last_column[i]);
        if (last_column[i] == delimiter) {
            delimiter_row = i;
        }

        size_t occ = occ_before.count(ch) ? occ_before[ch] : 0;
        occ_table[i] = occ;
        occ_before[ch] = occ + 1;
    }

    // C(c): index of the first occurrence of `c` in the sorted first column.
    std::map<unsigned char, size_t> first_occurrence;
    for (const auto& entry : occ_before) {
        first_occurrence[entry.first] = 0;
    }

    size_t total = 0;
    for (auto& entry : first_occurrence) {
        unsigned char ch = entry.first;
        entry.second = total;
        total += occ_before[ch];
    }

    // Follow Langmead's pseudocode: rebuild string by iteratively applying LF.
    std::vector<char> result;
    size_t row = delimiter_row;

    while (1) {
        unsigned char next_char = static_cast<unsigned char>(bwt_str[row]);
        row = first_occurrence[next_char] + occ_table[row];
        unsigned char ch = static_cast<unsigned char>(bwt_str[row]);
        if (ch == static_cast<unsigned char>(delimiter)) {
            break;
        }
        result.push_back(static_cast<char>(ch));
    }
    // Reverse the result and join as a string
    std::reverse(result.begin(), result.end());
    return std::string(result.begin(), result.end());    
}

struct Chunk {
    size_t index;
    std::string data;
};

// Writer thread function: writes inverse-BWT-transformed chunks in order
static void writer_thread_function_inverse(FileProcessor& processor, ReorderBuffer<Chunk>& reorder_buffer) {
    Chunk out_chunk;
    while (reorder_buffer.get_next(out_chunk)) {
        processor.write_chunk(out_chunk.data);
    }
}

// Worker thread function: consume BWT chunks, apply inverse BWT, push into reorder buffer
static void worker_thread_function_inverse(BlockingQueue<Chunk>& work_queue, ReorderBuffer<Chunk>& reorder_buffer, char delimiter) {
    Chunk in_chunk;
    while (work_queue.pop(in_chunk)) {
        // Apply inverse BWT to this chunk
        std::string result = bwt_inverse(in_chunk.data, delimiter);

        Chunk out_chunk;
        out_chunk.index = in_chunk.index;
        out_chunk.data = std::move(result);

        // Place result into reorder buffer
        reorder_buffer.put(out_chunk.index, out_chunk);
    }
}

// Process file with inverse BWT transform (multi-threaded over chunks)
int bwt_inverse_process_file(const char* input_file, const char* output_file, size_t block_size) {
    // Note: Forward BWT outputs chunks of size (input_size + 1) due to delimiter
    // So we need to read chunks of size (block_size + 1) to match
    size_t bwt_chunk_size = block_size + 1;
    
    FileProcessor processor(input_file, output_file, bwt_chunk_size);
    
    if (!processor.is_open()) {
        return 1;
    }
    
    char delimiter;
    if (!processor.read_char(delimiter)) {
        std::cerr << "Error: Empty input file" << std::endl;
        processor.close();
        return 1;
    }

    // Decide number of worker threads
    unsigned int num_workers = std::thread::hardware_concurrency();
    if (num_workers == 0) {
        num_workers = 4; // reasonable default
    }

    // Bounded queue of input chunks for workers
    BlockingQueue<Chunk> work_queue;

    // Reorder buffer to deliver transformed chunks in-order to writer
    const size_t reorder_capacity = num_workers * 4; // number of chunks allowed in flight
    ReorderBuffer<Chunk> reorder_buffer(reorder_capacity);

    std::atomic<size_t> next_chunk_index{0};

    // Writer thread: writes inverse-BWT-transformed chunks in order
    std::thread writer_thread(writer_thread_function_inverse, std::ref(processor), std::ref(reorder_buffer));

    // Worker threads: consume BWT chunks, apply inverse BWT, push into reorder buffer
    std::vector<std::thread> workers;
    workers.reserve(num_workers);

    for (unsigned int i = 0; i < num_workers; ++i) {
        workers.emplace_back(worker_thread_function_inverse, std::ref(work_queue), std::ref(reorder_buffer), delimiter);
    }

    // Main thread: read BWT chunks from input and enqueue work
    while (processor.has_more_data()) {
        std::string chunk = processor.read_chunk();

        if (chunk.empty()) {
            break;
        }

        Chunk work_chunk;
        work_chunk.index = next_chunk_index++;
        work_chunk.data = std::move(chunk);

        work_queue.push(work_chunk);
    }

    // No more work; let workers drain and exit
    work_queue.close();

    // Wait for all workers to finish and flush their results into the reorder buffer
    for (auto& t : workers) {
        if (t.joinable()) {
            t.join();
        }
    }

    // All results have been produced; close the reorder buffer so writer can finish
    reorder_buffer.close();

    if (writer_thread.joinable()) {
        writer_thread.join();
    }

    processor.close();
    return 0;
}

// Standalone CLI entry point (mirrors bwt.cpp)
#ifndef BUILD_TESTS
int main(int argc, char* argv[]) {
    // Check for command line arguments
    if (argc < 3 || argc > 4) {
        std::cerr << "Usage: " << argv[0] << " <input_file> <output_file> [block_size]" << std::endl;
        std::cerr << "  block_size: size of each block in bytes (default: 128)" << std::endl;
        return 1;
    }
    
    // Parse block size (default 128B)
    size_t block_size = 128;
    if (argc == 4) {
        block_size = std::stoul(argv[3]);
        if (block_size == 0) {
            std::cerr << "Error: Block size must be greater than 0" << std::endl;
            return 1;
        }
    }
    
    // Process the file using the multi-threaded inverse BWT
    return bwt_inverse_process_file(argv[1], argv[2], block_size);
}
#endif // BUILD_TESTS

