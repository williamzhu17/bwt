#include "bwt.hpp"
#include "file_processor.hpp"
#include <iostream>
#include <vector>
#include <algorithm>
#include <string>
#include <fstream>
#include <sstream>
#include <cstring>
#include <unordered_set>
#include <thread>
#include <atomic>

#include "../util/blocking_queue.hpp"
#include "../util/reorder_buffer.hpp"

// Find a byte value (0-255) that does not appear in the file.
// Returns the first unused byte value, or -1 if all 256 values are used.
int find_unique_char(const char* file_path) {
    std::unordered_set<unsigned char> used_bytes;
    const size_t chunk_size = 8192;  // 8KB chunks
    
    FileProcessor processor(file_path, "", chunk_size);
    if (!processor.is_open()) {
        return -1;
    }
    
    // Read file in chunks and track which bytes appear
    while (processor.has_more_data()) {
        std::string chunk = processor.read_chunk();
        
        for (char c : chunk) {
            used_bytes.insert(static_cast<unsigned char>(c));
        }
    }
    processor.close();
    
    // Find the first unused byte value (0-255)
    for (int byte_val = 0; byte_val < 256; ++byte_val) {
        if (used_bytes.find(static_cast<unsigned char>(byte_val)) == used_bytes.end()) {
            return byte_val;
        }
    }
    
    // All 256 byte values are used
    return -1;
}

// SA-IS implementation
namespace {

    std::vector<int> compute_bucket_sizes(const std::vector<int>& s, int sigma) {
        std::vector<int> bucket_sizes(sigma, 0);

        for (std::size_t i = 0; i < s.size(); ++i) {
            ++bucket_sizes[s[i]];
        }

        return bucket_sizes;
    }

    void bucket_bounds(
        const std::vector<int>& bucket_sizes,
        std::vector<int>& heads,
        std::vector<int>& tails) 
    {
        const int sigma = static_cast<int>(bucket_sizes.size());

        heads.resize(sigma);
        tails.resize(sigma);

        int sum = 0;

        for (int i = 0; i < sigma; ++i) {
            heads[i] = sum;
            sum += bucket_sizes[i];
            tails[i] = sum - 1;
        }
    }

    std::vector<int> induce_sort(
        const std::vector<int>& s,
        const std::vector<bool>& is_s_type,
        const std::vector<int>& lms_order,
        const std::vector<int>& bucket_sizes
    ) {
        const size_t n = s.size();
        std::vector<int> sa(n, -1);
        std::vector<int> heads, tails;
        bucket_bounds(bucket_sizes, heads, tails);

        // place LMS at bucket tails
        for (auto it = lms_order.rbegin(); it != lms_order.rend(); ++it) {
            int pos = *it;
            int bucket = s[pos];
            sa[tails[bucket]] = pos;
            --tails[bucket];
        }

        // induce L-type
        bucket_bounds(bucket_sizes, heads, tails);
        for (size_t i = 0; i < n; ++i) {
            int j = sa[i] - 1;
            if (j >= 0 && !is_s_type[j]) {
                int bucket = s[j];
                sa[heads[bucket]] = j;
                ++heads[bucket];
            }
        }

        // induce S-type
        bucket_bounds(bucket_sizes, heads, tails);
        for (int i = static_cast<int>(n) - 1; i >= 0; --i) {
            int j = sa[i] - 1;
            if (j >= 0 && is_s_type[j]) {
                int bucket = s[j];
                sa[tails[bucket]] = j;
                --tails[bucket];
            }
        }

        return sa;
    }

    std::vector<int> sais(const std::vector<int>& s, int sigma) {
        const size_t n = s.size();
        if (n == 0) return {};
        if (n == 1) return {0};

        std::vector<bool> is_s_type(n, true);

        for (int i = static_cast<int>(n) - 2; i >= 0; --i) {
            if (s[i] == s[i + 1]) {
                is_s_type[i] = is_s_type[i + 1];
            } else {
                is_s_type[i] = s[i] < s[i + 1];
            }
        }
        auto is_lms = [&](int idx) {
            return idx > 0 && is_s_type[idx] && !is_s_type[idx - 1];
        };

        std::vector<int> lms_positions;
        for (int i = 1; i < static_cast<int>(n); ++i) {
            if (is_lms(i)) {
                lms_positions.push_back(i);
            }
        }

        const std::vector<int> bucket_sizes = compute_bucket_sizes(s, sigma);
        std::vector<int> sa = induce_sort(s, is_s_type, lms_positions, bucket_sizes);

        // collect LMS in SA order
        std::vector<int> sorted_lms;
        sorted_lms.reserve(lms_positions.size());
        for (int idx : sa) {
            if (is_lms(idx)) {
                sorted_lms.push_back(idx);
            }
        }

        // name LMS substrings
        std::vector<int> lms_name(n, -1);
        int current_name = 0;
        lms_name[sorted_lms.front()] = current_name;
        for (size_t i = 1; i < sorted_lms.size(); ++i) {
            int a = sorted_lms[i - 1];
            int b = sorted_lms[i];
            bool same = true;
            while (true) {
                if (s[a] != s[b]) {
                    same = false;
                    break;
                }

                a++;
                b++;

                bool a_lms = is_lms(a);
                bool b_lms = is_lms(b);

                if (a_lms && b_lms) {
                    break;
                }

                if (a_lms != b_lms) {
                    same = false;
                    break;
                }
            }

            if (!same) {
                current_name++;
            }

            lms_name[sorted_lms[i]] = current_name;
        }

        const int num_names = current_name + 1;
        std::vector<int> reduced_string;
        reduced_string.reserve(lms_positions.size());

        for (int pos : lms_positions) {
            reduced_string.push_back(lms_name[pos]);
        }

        std::vector<int> reduced_sa;
        if (num_names == static_cast<int>(lms_positions.size())) {
            reduced_sa.resize(lms_positions.size());
            
            for (size_t i = 0; i < reduced_string.size(); ++i) {
                reduced_sa[reduced_string[i]] = static_cast<int>(i);
            }
        } else {
            reduced_sa = sais(reduced_string, num_names);
        }

        // rebuild LMS order from reduced SA
        std::vector<int> new_lms_order(lms_positions.size());
        for (size_t i = 0; i < reduced_sa.size(); ++i) {
            new_lms_order[i] = lms_positions[reduced_sa[i]];
        }

        sa = induce_sort(s, is_s_type, new_lms_order, bucket_sizes);
        return sa;
    }

} // namespace

// Build suffix array using SA-IS algorithm (O(n))
std::vector<size_t> build_suffix_array(const std::string& input) {
    const size_t n = input.size();
    // Shift characters by 1 so sentinel 0 is unique smallest.
    std::vector<int> data(n + 1);
    for (size_t i = 0; i < n; ++i) {
        data[i] = static_cast<unsigned char>(input[i]) + 1;
    }
    data[n] = 0; // sentinel

    const int sigma = 257; // 0 sentinel + 256 byte values shifted by 1
    std::vector<int> sa_int = sais(data, sigma);

    std::vector<size_t> sa;
    sa.reserve(n);
    for (int idx : sa_int) {
        if (idx != static_cast<int>(n)) {
            sa.push_back(static_cast<size_t>(idx));
        }
    }
    return sa;
}

// Forward BWT transform
std::string bwt_forward(const std::string& input, char delimiter) {
    std::string s = input + delimiter;
    size_t n = s.length();
    
    // Build suffix array for input+delimiter
    std::vector<size_t> sa = build_suffix_array(s);
    
    // Construct BWT string
    std::string bwt_str;
    bwt_str.resize(n);
    
    for (size_t i = 0; i < n; i++) {
        // BWT[i] is the character preceding the i-th sorted suffix
        if (sa[i] == 0) {
            bwt_str[i] = s[n - 1];
        } else {
            bwt_str[i] = s[sa[i] - 1];
        }
    }
    
    return bwt_str;
}

struct Chunk {
    size_t index;
    std::string data;
};

// Writer thread function: writes delimiter then BWT-transformed chunks in order
static void writer_thread_function(FileProcessor& processor, ReorderBuffer<Chunk>& reorder_buffer, char delimiter) {
    // Write delimiter byte as first byte of output file
    std::string delimiter_str(1, delimiter);
    processor.write_chunk(delimiter_str);

    Chunk out_chunk;
    while (reorder_buffer.get_next(out_chunk)) {
        processor.write_chunk(out_chunk.data);
    }
}

// Worker thread function: consume raw chunks, apply BWT, push into reorder buffer
static void worker_thread_function(BlockingQueue<Chunk>& work_queue, ReorderBuffer<Chunk>& reorder_buffer, char delimiter) {
    Chunk in_chunk;
    while (work_queue.pop(in_chunk)) {
        // Apply BWT to this chunk
        std::string result = bwt_forward(in_chunk.data, delimiter);

        Chunk out_chunk;
        out_chunk.index = in_chunk.index;
        out_chunk.data = std::move(result);

        // Place result into reorder buffer
        reorder_buffer.put(out_chunk.index, out_chunk);
    }
}

// Process file with forward BWT transform (multi-threaded over chunks)
int bwt_forward_process_file(const char* input_file, const char* output_file, size_t block_size) {
    // Find unique delimiter
    int delimiter_byte = find_unique_char(input_file);
    
    if (delimiter_byte == -1) {
        std::cerr << "Error: Cannot find a unique delimiter (all 256 byte values appear in file)" << std::endl;
        return 1;
    }
    
    char delimiter = static_cast<char>(delimiter_byte);
    
    // Create FileProcessor to handle file I/O
    FileProcessor processor(input_file, output_file, block_size);
    
    if (!processor.is_open()) {
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

    // Writer thread: writes delimiter then BWT-transformed chunks in order
    std::thread writer_thread(writer_thread_function, std::ref(processor), std::ref(reorder_buffer), delimiter);

    // Worker threads: consume raw chunks, apply BWT, push into reorder buffer
    std::vector<std::thread> workers;
    workers.reserve(num_workers);

    for (unsigned int i = 0; i < num_workers; ++i) {
        workers.emplace_back(worker_thread_function, std::ref(work_queue), std::ref(reorder_buffer), delimiter);
    }

    // Main thread: read chunks from input and enqueue work
    while (processor.has_more_data()) {
        std::string chunk_data = processor.read_chunk();

        if (chunk_data.empty()) {
            break;
        }

        Chunk chunk;
        chunk.index = next_chunk_index++;
        chunk.data = std::move(chunk_data);

        work_queue.push(chunk);
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

#ifndef BUILD_TESTS
int main(int argc, char* argv[]) {
    // Check for command line arguments
    if (argc < 3 || argc > 4) {
        std::cerr << "Usage: " << argv[0] << " <input_file> <output_file> [block_size]" << std::endl;
        std::cerr << "  block_size: size of each block in bytes (default: 65536)" << std::endl;
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
    
    // Process the file
    return bwt_forward_process_file(argv[1], argv[2], block_size);
}
#endif // BUILD_TESTS
