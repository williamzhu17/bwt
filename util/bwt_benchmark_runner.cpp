/*
 * Benchmark runner for BWT performance testing
 * Handles execution and timing of BWT operations
 */

#include "bwt_benchmark_runner.hpp"
#include "bzip2_bwt_utils.hpp"
#include "performance_comparison.hpp"
#include "file_utils.hpp"
#include "../src/bwt.hpp"
#include "../src/inverse_bwt.hpp"
#include <cstdio>

void TrialTempFiles::cleanup() const {
    std::remove(your_forward_output.c_str());
    std::remove(your_inverse_output.c_str());
    std::remove(bzip2_forward_output.c_str());
    std::remove(bzip2_inverse_output.c_str());
}

bool BWTBenchmarkRunner::run_your_forward_bwt(const std::string& input_file, const std::string& output_file,
                                                size_t block_size, double& elapsed_time, size_t& output_size) {
    Timer timer;
    timer.start();
    
    int result = bwt_forward_process_file(input_file.c_str(), output_file.c_str(), block_size);
    
    elapsed_time = timer.stop();
    
    if (result != 0) {
        return false;
    }
    
    output_size = get_file_size(output_file);
    return true;
}

bool BWTBenchmarkRunner::run_your_inverse_bwt(const std::string& input_file, const std::string& output_file,
                                                size_t block_size, double& elapsed_time) {
    Timer timer;
    timer.start();
    
    int result = bwt_inverse_process_file(input_file.c_str(), output_file.c_str(), block_size);
    
    elapsed_time = timer.stop();
    
    return (result == 0);
}

bool BWTBenchmarkRunner::run_bzip2_forward_bwt(const std::string& input_file, const std::string& output_file,
                                                 size_t block_size, double& elapsed_time, size_t& output_size) {
    Timer timer;
    timer.start();
    
    int result = Bzip2BWTProcessor::process_file_forward(input_file.c_str(), output_file.c_str(), block_size);
    
    elapsed_time = timer.stop();
    
    if (result != 0) {
        return false;
    }
    
    output_size = get_file_size(output_file);
    return true;
}

bool BWTBenchmarkRunner::run_bzip2_inverse_bwt(const std::string& input_file, const std::string& output_file,
                                                 size_t block_size, double& elapsed_time) {
    Timer timer;
    timer.start();
    
    int result = Bzip2BWTProcessor::process_file_inverse(input_file.c_str(), output_file.c_str(), block_size);
    
    elapsed_time = timer.stop();
    
    return (result == 0);
}

bool BWTBenchmarkRunner::run_single_trial(const std::string& input_file, size_t block_size,
                                           const TrialTempFiles& temp_files, TrialResult& trial_result) {
    // Clean up previous outputs
    temp_files.cleanup();
    
    // Forward BWT
    bool your_forward_success = run_your_forward_bwt(input_file, temp_files.your_forward_output, block_size,
                                                     trial_result.your_forward_time,
                                                     trial_result.your_forward_output_size);
    
    bool bzip2_forward_success = run_bzip2_forward_bwt(input_file, temp_files.bzip2_forward_output, block_size,
                                                       trial_result.bzip2_forward_time,
                                                       trial_result.bzip2_forward_output_size);
    
    if (!your_forward_success || !bzip2_forward_success) {
        return false;
    }
    
    // Inverse BWT
    bool your_inverse_success = run_your_inverse_bwt(temp_files.your_forward_output, temp_files.your_inverse_output,
                                                     block_size, trial_result.your_inverse_time);
    
    bool bzip2_inverse_success = run_bzip2_inverse_bwt(temp_files.bzip2_forward_output, temp_files.bzip2_inverse_output,
                                                       block_size, trial_result.bzip2_inverse_time);
    
    if (!your_inverse_success || !bzip2_inverse_success) {
        return false;
    }
    
    // Calculate metrics
    trial_result.calculate_metrics();
    
    return true;
}

