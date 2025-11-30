/*
 * Performance comparison between your BWT and bzip2's BWT
 * 
 * This program runs both implementations and compares their performance
 */

#include <iostream>
#include <string>
#include <vector>
#include <cstdio>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <cstring>
#include <algorithm>
#include <numeric>
#include <cmath>
#include <fstream>
#include <map>
#include "../src/file_processor.hpp"
#include "../src/bwt.hpp"
#include "../src/inverse_bwt.hpp"
#include "../util/file_utils.hpp"
#include "../util/format_utils.hpp"

// Include bzip2 headers for inline calls
extern "C" {
#include "../bzip2/bzlib_private.h"
}

// Forward declarations from bzip2 (will be linked from bzlib.c, blocksort.c, etc.)
extern "C" {
    void BZ2_blockSort(EState* s);
    void BZ2_bz__AssertH__fail(int errcode);
    extern UInt32 BZ2_crc32Table[256];
}

// Configuration
const int DEFAULT_NUM_TRIALS = 5;

// Default block sizes for performance tests
// Note: bzip2 allocates space in multiples of 100KB, but can process any size up to the allocated maximum
// The conversion logic automatically calculates the minimum blockSize100k needed
const std::vector<size_t> DEFAULT_BLOCK_SIZES = {65536, 131072, 262144};  // 64KB, 128KB, 256KB

// ===== bzip2 BWT helper functions (for inline calls) =====

// Initialize EState for BWT only (no compression)
static EState* init_bwt_state(int blockSize100k, int verbosity, int workFactor) {
    EState* s = (EState*)malloc(sizeof(EState));
    if (!s) return NULL;
    
    memset(s, 0, sizeof(EState));
    
    Int32 n = 100000 * blockSize100k;
    s->arr1 = (UInt32*)malloc(n * sizeof(UInt32));
    s->arr2 = (UInt32*)malloc((n + BZ_N_OVERSHOOT) * sizeof(UInt32));
    s->ftab = (UInt32*)malloc(65537 * sizeof(UInt32));
    
    if (!s->arr1 || !s->arr2 || !s->ftab) {
        if (s->arr1) free(s->arr1);
        if (s->arr2) free(s->arr2);
        if (s->ftab) free(s->ftab);
        free(s);
        return NULL;
    }
    
    s->blockSize100k = blockSize100k;
    s->nblockMAX = 100000 * blockSize100k - 19;
    s->verbosity = verbosity;
    s->workFactor = workFactor;
    
    s->block = (UChar*)s->arr2;
    s->ptr = (UInt32*)s->arr1;
    
    return s;
}

// Free EState
static void free_bwt_state(EState* s) {
    if (!s) return;
    if (s->arr1) free(s->arr1);
    if (s->arr2) free(s->arr2);
    if (s->ftab) free(s->ftab);
    free(s);
}

// Extract BWT output from sorted block
static void extract_bwt_output(EState* s, std::vector<UChar>& bwt_output) {
    UInt32* ptr = s->ptr;
    UChar* block = s->block;
    Int32 nblock = s->nblock;
    
    // Reserve space for BWT output
    bwt_output.resize(nblock);
    
    // Extract BWT: for each position i in sorted order,
    // BWT[i] is the character before the suffix starting at ptr[i]
    for (Int32 i = 0; i < nblock; i++) {
        Int32 pos = ptr[i];
        if (pos == 0) {
            // Wrap around: character before position 0 is at end
            bwt_output[i] = block[nblock - 1];
        } else {
            bwt_output[i] = block[pos - 1];
        }
    }
}

// Process a single block
static bool process_block(EState* s, const UChar* data, Int32 size, 
                          std::vector<UChar>& bwt_output, Int32& origPtr) {
    if (size > s->nblockMAX) {
        std::cerr << "Error: Block size " << size << " exceeds maximum " 
                  << s->nblockMAX << std::endl;
        return false;
    }
    
    // Copy data into block
    s->nblock = size;
    memcpy(s->block, data, size);
    
    // Initialize inUse array
    memset(s->inUse, 0, 256);
    for (Int32 i = 0; i < size; i++) {
        s->inUse[data[i]] = True;
    }
    
    // Perform BWT
    BZ2_blockSort(s);
    
    // Extract BWT output
    extract_bwt_output(s, bwt_output);
    
    // Get original pointer
    origPtr = s->origPtr;
    
    return true;
}

// Process file with BWT (inline version)
int bzip2_bwt_process_file(const char* input_file, const char* output_file, 
                           size_t block_size) {
    std::ifstream in(input_file, std::ios::binary);
    if (!in.is_open()) {
        std::cerr << "Error: Cannot open input file: " << input_file << std::endl;
        return 1;
    }
    
    std::ofstream out(output_file, std::ios::binary);
    if (!out.is_open()) {
        std::cerr << "Error: Cannot open output file: " << output_file << std::endl;
        return 1;
    }
    
    // Convert block_size to bzip2's blockSize100k format
    // bzip2 uses blocks of 100k * blockSize100k bytes
    // nblockMAX = 100000 * blockSize100k - 19
    // We need to find the minimum blockSize100k such that block_size <= nblockMAX
    // block_size <= 100000 * blockSize100k - 19
    // block_size + 19 <= 100000 * blockSize100k
    // blockSize100k >= ceil((block_size + 19) / 100000)
    int blockSize100k = ((block_size + 19) + 99999) / 100000;  // Ceiling division
    if (blockSize100k < 1) blockSize100k = 1;
    if (blockSize100k > 9) blockSize100k = 9;  // bzip2 max is 9
    
    // Initialize BWT state
    EState* s = init_bwt_state(blockSize100k, 0, 30);
    if (!s) {
        std::cerr << "Error: Failed to initialize BWT state" << std::endl;
        return 1;
    }
    
    // Read and process blocks
    std::vector<UChar> buffer(block_size);
    std::vector<UChar> bwt_output;
    
    while (in.good()) {
        // Read a block
        in.read((char*)buffer.data(), block_size);
        size_t bytes_read = in.gcount();
        
        if (bytes_read == 0) break;
        
        // Process block
        Int32 origPtr;
        if (!process_block(s, buffer.data(), bytes_read, bwt_output, origPtr)) {
            free_bwt_state(s);
            return 1;
        }
        
        // Write bzip2 format: marker byte (0xFF) + origPtr (3 bytes) + BWT output
        UChar marker = 0xFF;
        out.write((char*)&marker, 1);
        
        // Write origPtr (3 bytes, big-endian, matching bzip2's format)
        UChar origPtr_bytes[3];
        origPtr_bytes[0] = (origPtr >> 16) & 0xFF;
        origPtr_bytes[1] = (origPtr >> 8) & 0xFF;
        origPtr_bytes[2] = origPtr & 0xFF;
        out.write((char*)origPtr_bytes, 3);
        
        // Write BWT output (this is the actual BWT transform result)
        out.write((char*)bwt_output.data(), bwt_output.size());
    }
    
    free_bwt_state(s);
    return 0;
}

// Inverse BWT using origPtr (bzip2's approach)
std::string bzip2_inverse_bwt(const std::vector<unsigned char>& bwt_str, int origPtr) {
    size_t len = bwt_str.size();
    if (len == 0) return "";
    
    // Build occurrence table: Occ(c, i) = number of occurrences of c strictly before position i
    std::vector<size_t> occ_table(len, 0);
    std::map<unsigned char, size_t> occ_before;
    
    for (size_t i = 0; i < len; ++i) {
        unsigned char ch = bwt_str[i];
        size_t occ = occ_before.count(ch) ? occ_before[ch] : 0;
        occ_table[i] = occ;
        occ_before[ch] = occ + 1;
    }
    
    // C(c): index of the first occurrence of c in the sorted first column
    std::map<unsigned char, size_t> first_occurrence;
    size_t total = 0;
    for (const auto& entry : occ_before) {
        first_occurrence[entry.first] = total;
        total += entry.second;
    }
    
    // Reconstruct original string by following LF mapping starting from origPtr
    std::vector<char> result;
    size_t row = origPtr;
    
    for (size_t i = 0; i < len; ++i) {
        unsigned char ch = bwt_str[row];
        result.push_back(static_cast<char>(ch));
        row = first_occurrence[ch] + occ_table[row];
    }
    
    // Reverse to get original string (we built it backwards)
    std::reverse(result.begin(), result.end());
    return std::string(result.begin(), result.end());
}

// Process file with inverse BWT (reads bzip2 format, inline version)
int bzip2_inverse_bwt_process_file(const char* input_file, const char* output_file, 
                                   size_t block_size) {
    std::ifstream in(input_file, std::ios::binary);
    if (!in.is_open()) {
        std::cerr << "Error: Cannot open input file: " << input_file << std::endl;
        return 1;
    }
    
    std::ofstream out(output_file, std::ios::binary);
    if (!out.is_open()) {
        std::cerr << "Error: Cannot open output file: " << output_file << std::endl;
        return 1;
    }
    
    // Process blocks
    while (in.good()) {
        // Read marker byte (should be 0xFF)
        unsigned char marker;
        in.read((char*)&marker, 1);
        if (in.gcount() == 0) break;
        
        if (marker != 0xFF) {
            std::cerr << "Error: Invalid marker byte: 0x" << std::hex << (int)marker << std::endl;
            return 1;
        }
        
        // Read origPtr (3 bytes, big-endian)
        unsigned char origPtr_bytes[3];
        in.read((char*)origPtr_bytes, 3);
        if (in.gcount() != 3) {
            std::cerr << "Error: Failed to read origPtr" << std::endl;
            return 1;
        }
        
        int origPtr = (origPtr_bytes[0] << 16) | (origPtr_bytes[1] << 8) | origPtr_bytes[2];
        
        // Read BWT string (size should match block_size)
        std::vector<unsigned char> bwt_output(block_size);
        in.read((char*)bwt_output.data(), block_size);
        size_t bytes_read = in.gcount();
        
        if (bytes_read == 0) break;
        
        // Resize if we read less than block_size (last block)
        if (bytes_read < block_size) {
            bwt_output.resize(bytes_read);
        }
        
        // Validate origPtr
        if (origPtr < 0 || origPtr >= (int)bytes_read) {
            std::cerr << "Error: Invalid origPtr: " << origPtr << " (block size: " << bytes_read << ")" << std::endl;
            return 1;
        }
        
        // Apply inverse BWT
        std::string result = bzip2_inverse_bwt(bwt_output, origPtr);
        
        // Write result
        out.write(result.c_str(), result.size());
    }
    
    return 0;
}

// ===== End of bzip2 helper functions =====

struct TrialResult {
    // Forward BWT
    double your_forward_time = 0.0;
    double bzip2_forward_time = 0.0;
    size_t your_forward_output_size = 0;
    size_t bzip2_forward_output_size = 0;
    
    // Inverse BWT
    double your_inverse_time = 0.0;
    double bzip2_inverse_time = 0.0;
    
    // Round trip (forward + inverse)
    double your_roundtrip_time = 0.0;
    double bzip2_roundtrip_time = 0.0;
    
    // Comparison metrics
    double forward_speedup = 0.0;
    double inverse_speedup = 0.0;
    double roundtrip_speedup = 0.0;
};

struct ComparisonResult {
    std::string test_name;
    size_t block_size;
    size_t file_size;
    int num_trials = 0;
    
    // Trial results
    std::vector<TrialResult> trials;
    
    // Forward BWT statistics
    double your_forward_time_mean = 0.0;
    double your_forward_time_stddev = 0.0;
    double your_forward_time_min = 0.0;
    double your_forward_time_max = 0.0;
    
    double bzip2_forward_time_mean = 0.0;
    double bzip2_forward_time_stddev = 0.0;
    double bzip2_forward_time_min = 0.0;
    double bzip2_forward_time_max = 0.0;
    
    double forward_speedup_mean = 0.0;
    double forward_speedup_stddev = 0.0;
    
    // Inverse BWT statistics
    double your_inverse_time_mean = 0.0;
    double your_inverse_time_stddev = 0.0;
    double your_inverse_time_min = 0.0;
    double your_inverse_time_max = 0.0;
    
    double bzip2_inverse_time_mean = 0.0;
    double bzip2_inverse_time_stddev = 0.0;
    double bzip2_inverse_time_min = 0.0;
    double bzip2_inverse_time_max = 0.0;
    
    double inverse_speedup_mean = 0.0;
    double inverse_speedup_stddev = 0.0;
    
    // Round trip statistics
    double your_roundtrip_time_mean = 0.0;
    double your_roundtrip_time_stddev = 0.0;
    double your_roundtrip_time_min = 0.0;
    double your_roundtrip_time_max = 0.0;
    
    double bzip2_roundtrip_time_mean = 0.0;
    double bzip2_roundtrip_time_stddev = 0.0;
    double bzip2_roundtrip_time_min = 0.0;
    double bzip2_roundtrip_time_max = 0.0;
    
    double roundtrip_speedup_mean = 0.0;
    double roundtrip_speedup_stddev = 0.0;
    
    size_t your_forward_output_size = 0;
    size_t bzip2_forward_output_size = 0;
    
    void calculate_statistics() {
        if (trials.empty()) return;
        
        // Extract time vectors
        std::vector<double> your_forward_times, bzip2_forward_times, forward_speedups;
        std::vector<double> your_inverse_times, bzip2_inverse_times, inverse_speedups;
        std::vector<double> your_roundtrip_times, bzip2_roundtrip_times, roundtrip_speedups;
        
        for (const auto& trial : trials) {
            your_forward_times.push_back(trial.your_forward_time);
            bzip2_forward_times.push_back(trial.bzip2_forward_time);
            if (trial.forward_speedup > 0) {
                forward_speedups.push_back(trial.forward_speedup);
            }
            
            your_inverse_times.push_back(trial.your_inverse_time);
            bzip2_inverse_times.push_back(trial.bzip2_inverse_time);
            if (trial.inverse_speedup > 0) {
                inverse_speedups.push_back(trial.inverse_speedup);
            }
            
            your_roundtrip_times.push_back(trial.your_roundtrip_time);
            bzip2_roundtrip_times.push_back(trial.bzip2_roundtrip_time);
            if (trial.roundtrip_speedup > 0) {
                roundtrip_speedups.push_back(trial.roundtrip_speedup);
            }
        }
        
        // Calculate forward BWT statistics
        your_forward_time_mean = calculate_mean(your_forward_times);
        your_forward_time_stddev = calculate_stddev(your_forward_times, your_forward_time_mean);
        your_forward_time_min = *std::min_element(your_forward_times.begin(), your_forward_times.end());
        your_forward_time_max = *std::max_element(your_forward_times.begin(), your_forward_times.end());
        
        bzip2_forward_time_mean = calculate_mean(bzip2_forward_times);
        bzip2_forward_time_stddev = calculate_stddev(bzip2_forward_times, bzip2_forward_time_mean);
        bzip2_forward_time_min = *std::min_element(bzip2_forward_times.begin(), bzip2_forward_times.end());
        bzip2_forward_time_max = *std::max_element(bzip2_forward_times.begin(), bzip2_forward_times.end());
        
        if (!forward_speedups.empty()) {
            forward_speedup_mean = calculate_mean(forward_speedups);
            forward_speedup_stddev = calculate_stddev(forward_speedups, forward_speedup_mean);
        }
        
        // Calculate inverse BWT statistics
        your_inverse_time_mean = calculate_mean(your_inverse_times);
        your_inverse_time_stddev = calculate_stddev(your_inverse_times, your_inverse_time_mean);
        your_inverse_time_min = *std::min_element(your_inverse_times.begin(), your_inverse_times.end());
        your_inverse_time_max = *std::max_element(your_inverse_times.begin(), your_inverse_times.end());
        
        bzip2_inverse_time_mean = calculate_mean(bzip2_inverse_times);
        bzip2_inverse_time_stddev = calculate_stddev(bzip2_inverse_times, bzip2_inverse_time_mean);
        bzip2_inverse_time_min = *std::min_element(bzip2_inverse_times.begin(), bzip2_inverse_times.end());
        bzip2_inverse_time_max = *std::max_element(bzip2_inverse_times.begin(), bzip2_inverse_times.end());
        
        if (!inverse_speedups.empty()) {
            inverse_speedup_mean = calculate_mean(inverse_speedups);
            inverse_speedup_stddev = calculate_stddev(inverse_speedups, inverse_speedup_mean);
        }
        
        // Calculate round trip statistics
        your_roundtrip_time_mean = calculate_mean(your_roundtrip_times);
        your_roundtrip_time_stddev = calculate_stddev(your_roundtrip_times, your_roundtrip_time_mean);
        your_roundtrip_time_min = *std::min_element(your_roundtrip_times.begin(), your_roundtrip_times.end());
        your_roundtrip_time_max = *std::max_element(your_roundtrip_times.begin(), your_roundtrip_times.end());
        
        bzip2_roundtrip_time_mean = calculate_mean(bzip2_roundtrip_times);
        bzip2_roundtrip_time_stddev = calculate_stddev(bzip2_roundtrip_times, bzip2_roundtrip_time_mean);
        bzip2_roundtrip_time_min = *std::min_element(bzip2_roundtrip_times.begin(), bzip2_roundtrip_times.end());
        bzip2_roundtrip_time_max = *std::max_element(bzip2_roundtrip_times.begin(), bzip2_roundtrip_times.end());
        
        if (!roundtrip_speedups.empty()) {
            roundtrip_speedup_mean = calculate_mean(roundtrip_speedups);
            roundtrip_speedup_stddev = calculate_stddev(roundtrip_speedups, roundtrip_speedup_mean);
        }
        
        // Output sizes (should be same across trials, use first)
        if (!trials.empty()) {
            your_forward_output_size = trials[0].your_forward_output_size;
            bzip2_forward_output_size = trials[0].bzip2_forward_output_size;
        }
    }
    
private:
    double calculate_mean(const std::vector<double>& values) {
        if (values.empty()) return 0.0;
        return std::accumulate(values.begin(), values.end(), 0.0) / values.size();
    }
    
    double calculate_stddev(const std::vector<double>& values, double mean) {
        if (values.size() <= 1) return 0.0;
        double sum_squared_diff = 0.0;
        for (double val : values) {
            double diff = val - mean;
            sum_squared_diff += diff * diff;
        }
        return std::sqrt(sum_squared_diff / (values.size() - 1));
    }
};

// Run your forward BWT implementation
bool run_your_forward_bwt(const std::string& input_file, const std::string& output_file, 
                          size_t block_size, double& elapsed_time, size_t& output_size) {
    auto start = std::chrono::high_resolution_clock::now();
    
    int result = bwt_forward_process_file(input_file.c_str(), output_file.c_str(), block_size);
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    elapsed_time = duration.count() / 1000.0;  // Convert to milliseconds
    
    if (result != 0) {
        return false;
    }
    
    output_size = get_file_size(output_file);
    return true;
}

// Run your inverse BWT implementation
bool run_your_inverse_bwt(const std::string& input_file, const std::string& output_file,
                          size_t block_size, double& elapsed_time) {
    auto start = std::chrono::high_resolution_clock::now();
    
    int result = bwt_inverse_process_file(input_file.c_str(), output_file.c_str(), block_size);
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    elapsed_time = duration.count() / 1000.0;  // Convert to milliseconds
    
    return (result == 0);
}

// Run bzip2 forward BWT extractor (inline call)
bool run_bzip2_forward_bwt(const std::string& input_file, const std::string& output_file,
                           size_t block_size, double& elapsed_time, size_t& output_size) {
    auto start = std::chrono::high_resolution_clock::now();
    
    int result = bzip2_bwt_process_file(input_file.c_str(), output_file.c_str(), block_size);
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    elapsed_time = duration.count() / 1000.0;  // Convert to milliseconds
    
    if (result != 0) {
        return false;
    }
    
    output_size = get_file_size(output_file);
    return true;
}

// Run bzip2 inverse BWT extractor (inline call)
bool run_bzip2_inverse_bwt(const std::string& input_file, const std::string& output_file,
                            size_t block_size, double& elapsed_time) {
    auto start = std::chrono::high_resolution_clock::now();
    
    int result = bzip2_inverse_bwt_process_file(input_file.c_str(), output_file.c_str(), block_size);
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    elapsed_time = duration.count() / 1000.0;  // Convert to milliseconds
    
    return (result == 0);
}

// Compare implementations on a single file with multiple trials (forward, inverse, and round trip)
ComparisonResult compare_implementations(const std::string& input_file, 
                                         const std::string& test_name,
                                         size_t block_size,
                                         int num_trials) {
    ComparisonResult result;
    result.test_name = test_name;
    result.block_size = block_size;
    result.file_size = get_file_size(input_file);
    result.num_trials = num_trials;
    
    std::string your_forward_output = "build/tmp/your_forward.bwt";
    std::string your_inverse_output = "build/tmp/your_inverse.txt";
    std::string bzip2_forward_output = "build/tmp/bzip2_forward.bwt";
    std::string bzip2_inverse_output = "build/tmp/bzip2_inverse.txt";
    
    // Run multiple trials
    for (int trial = 0; trial < num_trials; trial++) {
        TrialResult trial_result;
        
        // Clean up previous outputs
        std::remove(your_forward_output.c_str());
        std::remove(your_inverse_output.c_str());
        std::remove(bzip2_forward_output.c_str());
        std::remove(bzip2_inverse_output.c_str());
        
        // === Forward BWT ===
        bool your_forward_success = run_your_forward_bwt(input_file, your_forward_output, block_size, 
                                                        trial_result.your_forward_time, 
                                                        trial_result.your_forward_output_size);
        
        bool bzip2_forward_success = run_bzip2_forward_bwt(input_file, bzip2_forward_output, block_size,
                                                            trial_result.bzip2_forward_time, 
                                                            trial_result.bzip2_forward_output_size);
        
        if (!your_forward_success) {
            std::cerr << "Warning: Your forward BWT failed for " << test_name << " (trial " << (trial + 1) << ")" << std::endl;
            continue;
        }
        
        if (!bzip2_forward_success) {
            std::cerr << "Warning: bzip2 forward BWT failed for " << test_name << " (trial " << (trial + 1) << ")" << std::endl;
            continue;
        }
        
        // === Inverse BWT ===
        bool your_inverse_success = run_your_inverse_bwt(your_forward_output, your_inverse_output, block_size,
                                                         trial_result.your_inverse_time);
        
        bool bzip2_inverse_success = run_bzip2_inverse_bwt(bzip2_forward_output, bzip2_inverse_output, block_size,
                                                           trial_result.bzip2_inverse_time);
        
        if (!your_inverse_success) {
            std::cerr << "Warning: Your inverse BWT failed for " << test_name << " (trial " << (trial + 1) << ")" << std::endl;
            continue;
        }
        
        if (!bzip2_inverse_success) {
            std::cerr << "Warning: bzip2 inverse BWT failed for " << test_name << " (trial " << (trial + 1) << ")" << std::endl;
            continue;
        }
        
        // === Round trip times ===
        trial_result.your_roundtrip_time = trial_result.your_forward_time + trial_result.your_inverse_time;
        trial_result.bzip2_roundtrip_time = trial_result.bzip2_forward_time + trial_result.bzip2_inverse_time;
        
        // === Calculate comparison metrics ===
        if (trial_result.your_forward_time > 0) {
            trial_result.forward_speedup = trial_result.bzip2_forward_time / trial_result.your_forward_time;
        }
        if (trial_result.your_inverse_time > 0) {
            trial_result.inverse_speedup = trial_result.bzip2_inverse_time / trial_result.your_inverse_time;
        }
        if (trial_result.your_roundtrip_time > 0) {
            trial_result.roundtrip_speedup = trial_result.bzip2_roundtrip_time / trial_result.your_roundtrip_time;
        }
        
        result.trials.push_back(trial_result);
    }
    
    // Calculate statistics
    result.calculate_statistics();
    
    // Clean up
    std::remove(your_forward_output.c_str());
    std::remove(your_inverse_output.c_str());
    std::remove(bzip2_forward_output.c_str());
    std::remove(bzip2_inverse_output.c_str());
    
    return result;
}

// Helper function to print timing statistics
void print_timing_stats(const std::string& label, double mean, double stddev, 
                       double min, double max, size_t num_trials) {
    std::cout << "  " << label << ":" << std::endl;
    std::cout << "    Time:      " << std::fixed << std::setprecision(3) << mean << " ms";
    if (num_trials > 1) {
        std::cout << " ± " << std::fixed << std::setprecision(3) << stddev << " ms";
    }
    std::cout << std::endl;
    std::cout << "    Min:       " << std::fixed << std::setprecision(3) << min << " ms" << std::endl;
    std::cout << "    Max:       " << std::fixed << std::setprecision(3) << max << " ms" << std::endl;
}

// Helper function to print comparison metrics
void print_comparison_metrics(const std::string& label, double speedup_mean, double speedup_stddev,
                              double your_time, double bzip2_time, size_t num_trials) {
    if (speedup_mean > 0) {
        std::cout << "  " << label << ":" << std::endl;
        std::cout << "    Speedup:   " << std::fixed << std::setprecision(3) << speedup_mean << "x";
        if (num_trials > 1) {
            std::cout << " ± " << std::fixed << std::setprecision(3) << speedup_stddev << "x";
        }
        if (speedup_mean < 1.0) {
            std::cout << " (bzip2 is " << std::fixed << std::setprecision(1) 
                      << (1.0 / speedup_mean - 1.0) * 100 << "% faster)";
        } else {
            std::cout << " (your BWT is " << std::fixed << std::setprecision(1) 
                      << (speedup_mean - 1.0) * 100 << "% faster)";
        }
        std::cout << std::endl;
        
        double time_diff = your_time - bzip2_time;
        std::cout << "    Time Diff: " << std::fixed << std::setprecision(3) << time_diff << " ms";
        if (time_diff > 0) {
            std::cout << " (bzip2 is faster)";
        } else {
            std::cout << " (your BWT is faster)";
        }
        std::cout << std::endl;
    }
}

// Print comparison results with statistics
void print_comparison(const ComparisonResult& result) {
    std::cout << "\n" << std::string(80, '=') << std::endl;
    std::cout << "Test: " << result.test_name << std::endl;
    std::cout << "Block Size: " << format_size(result.block_size) << std::endl;
    std::cout << "File Size: " << format_size(result.file_size) << std::endl;
    std::cout << "Trials: " << result.num_trials << " (successful: " << result.trials.size() << ")" << std::endl;
    std::cout << std::string(80, '-') << std::endl;
    
    if (result.trials.empty()) {
        std::cout << "ERROR: No successful trials!" << std::endl;
        std::cout << std::string(80, '=') << std::endl;
        return;
    }
    
    // === FORWARD BWT ===
    std::cout << "\nFORWARD BWT:" << std::endl;
    std::cout << "Your BWT:" << std::endl;
    print_timing_stats("Forward", result.your_forward_time_mean, result.your_forward_time_stddev,
                      result.your_forward_time_min, result.your_forward_time_max, result.trials.size());
    std::cout << "  Output:    " << format_size(result.your_forward_output_size) << std::endl;
    
    std::cout << "\nbzip2 BWT:" << std::endl;
    print_timing_stats("Forward", result.bzip2_forward_time_mean, result.bzip2_forward_time_stddev,
                      result.bzip2_forward_time_min, result.bzip2_forward_time_max, result.trials.size());
    std::cout << "  Output:    " << format_size(result.bzip2_forward_output_size) << std::endl;
    
    print_comparison_metrics("Comparison", result.forward_speedup_mean, result.forward_speedup_stddev,
                            result.your_forward_time_mean, result.bzip2_forward_time_mean, result.trials.size());
    
    // === INVERSE BWT ===
    std::cout << "\nINVERSE BWT:" << std::endl;
    std::cout << "Your BWT:" << std::endl;
    print_timing_stats("Inverse", result.your_inverse_time_mean, result.your_inverse_time_stddev,
                      result.your_inverse_time_min, result.your_inverse_time_max, result.trials.size());
    
    std::cout << "\nbzip2 BWT:" << std::endl;
    print_timing_stats("Inverse", result.bzip2_inverse_time_mean, result.bzip2_inverse_time_stddev,
                      result.bzip2_inverse_time_min, result.bzip2_inverse_time_max, result.trials.size());
    
    print_comparison_metrics("Comparison", result.inverse_speedup_mean, result.inverse_speedup_stddev,
                            result.your_inverse_time_mean, result.bzip2_inverse_time_mean, result.trials.size());
    
    // === ROUND TRIP ===
    std::cout << "\nROUND TRIP (Forward + Inverse):" << std::endl;
    std::cout << "Your BWT:" << std::endl;
    print_timing_stats("Round Trip", result.your_roundtrip_time_mean, result.your_roundtrip_time_stddev,
                      result.your_roundtrip_time_min, result.your_roundtrip_time_max, result.trials.size());
    
    std::cout << "\nbzip2 BWT:" << std::endl;
    print_timing_stats("Round Trip", result.bzip2_roundtrip_time_mean, result.bzip2_roundtrip_time_stddev,
                      result.bzip2_roundtrip_time_min, result.bzip2_roundtrip_time_max, result.trials.size());
    
    print_comparison_metrics("Comparison", result.roundtrip_speedup_mean, result.roundtrip_speedup_stddev,
                            result.your_roundtrip_time_mean, result.bzip2_roundtrip_time_mean, result.trials.size());
    
    // Throughput for round trip
    if (result.roundtrip_speedup_mean > 0) {
        double your_throughput = (result.file_size / (1024.0 * 1024.0)) / (result.your_roundtrip_time_mean / 1000.0);
        double bzip2_throughput = (result.file_size / (1024.0 * 1024.0)) / (result.bzip2_roundtrip_time_mean / 1000.0);
        
        std::cout << "  Throughput:" << std::endl;
        std::cout << "    Your BWT:  " << std::fixed << std::setprecision(2) 
                  << your_throughput << " MB/s" << std::endl;
        std::cout << "    bzip2 BWT: " << std::fixed << std::setprecision(2) 
                  << bzip2_throughput << " MB/s" << std::endl;
    }
    
    std::cout << std::string(80, '=') << std::endl;
    
    // Output summary lines for aggregation
    // Format: SUMMARY|test_name|phase|your_time_mean|bzip2_time_mean|speedup|winner|faster_by_pct
    if (result.forward_speedup_mean > 0) {
        std::string winner = (result.forward_speedup_mean < 1.0) ? "bzip2" : "your_bwt";
        double speedup_pct = (result.forward_speedup_mean < 1.0) 
            ? (1.0 / result.forward_speedup_mean - 1.0) * 100.0
            : (result.forward_speedup_mean - 1.0) * 100.0;
        std::cout << "SUMMARY|" << result.test_name << "|forward|" 
                  << std::fixed << std::setprecision(3) << result.your_forward_time_mean << "|"
                  << std::fixed << std::setprecision(3) << result.bzip2_forward_time_mean << "|"
                  << std::fixed << std::setprecision(3) << result.forward_speedup_mean << "|"
                  << winner << "|"
                  << std::fixed << std::setprecision(1) << speedup_pct << std::endl;
    }
    
    if (result.inverse_speedup_mean > 0) {
        std::string winner = (result.inverse_speedup_mean < 1.0) ? "bzip2" : "your_bwt";
        double speedup_pct = (result.inverse_speedup_mean < 1.0) 
            ? (1.0 / result.inverse_speedup_mean - 1.0) * 100.0
            : (result.inverse_speedup_mean - 1.0) * 100.0;
        std::cout << "SUMMARY|" << result.test_name << "|inverse|" 
                  << std::fixed << std::setprecision(3) << result.your_inverse_time_mean << "|"
                  << std::fixed << std::setprecision(3) << result.bzip2_inverse_time_mean << "|"
                  << std::fixed << std::setprecision(3) << result.inverse_speedup_mean << "|"
                  << winner << "|"
                  << std::fixed << std::setprecision(1) << speedup_pct << std::endl;
    }
    
    if (result.roundtrip_speedup_mean > 0) {
        std::string winner = (result.roundtrip_speedup_mean < 1.0) ? "bzip2" : "your_bwt";
        double speedup_pct = (result.roundtrip_speedup_mean < 1.0) 
            ? (1.0 / result.roundtrip_speedup_mean - 1.0) * 100.0
            : (result.roundtrip_speedup_mean - 1.0) * 100.0;
        std::cout << "SUMMARY|" << result.test_name << "|roundtrip|" 
                  << std::fixed << std::setprecision(3) << result.your_roundtrip_time_mean << "|"
                  << std::fixed << std::setprecision(3) << result.bzip2_roundtrip_time_mean << "|"
                  << std::fixed << std::setprecision(3) << result.roundtrip_speedup_mean << "|"
                  << winner << "|"
                  << std::fixed << std::setprecision(1) << speedup_pct << std::endl;
    }
}

int main(int argc, char* argv[]) {
    std::cout << "\n" << std::string(80, '=') << std::endl;
    std::cout << "BWT Performance Comparison: Your Implementation vs bzip2" << std::endl;
    std::cout << "Testing: Forward BWT, Inverse BWT, and Round Trip" << std::endl;
    std::cout << std::string(80, '=') << std::endl;
    
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <input_file>" << std::endl;
        std::cerr << "  input_file: File to test" << std::endl;
        std::cerr << "  Note: Using default block sizes: 64KB, 128KB, 256KB" << std::endl;
        return 1;
    }
    
    std::string input_file = argv[1];
    
    // Check if input file exists
    if (!file_exists(input_file)) {
        std::cerr << "Error: Input file not found: " << input_file << std::endl;
        return 1;
    }
    
    // Create temp directory
    if (!create_directory("build/tmp")) {
        std::cerr << "Error: Failed to create build/tmp directory" << std::endl;
        return 1;
    }
    
    // Extract test name from file path
    std::string test_name = input_file;
    size_t last_slash = test_name.find_last_of("/\\");
    if (last_slash != std::string::npos) {
        test_name = test_name.substr(last_slash + 1);
    }
    
    // Run comparison for each default block size
    for (size_t block_size : DEFAULT_BLOCK_SIZES) {
        std::cout << "\n" << std::string(80, '=') << std::endl;
        std::cout << "Block Size: " << format_size(block_size) << std::endl;
        std::cout << std::string(80, '=') << std::endl;
        
        std::cout << "Running " << DEFAULT_NUM_TRIALS << " trial(s)..." << std::endl;
        ComparisonResult result = compare_implementations(input_file, test_name, block_size, DEFAULT_NUM_TRIALS);
        
        // Print results
        print_comparison(result);
    }
    
    return 0;
}

