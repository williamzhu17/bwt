#ifndef BWT_BENCHMARK_RUNNER_HPP
#define BWT_BENCHMARK_RUNNER_HPP

#include "performance_comparison.hpp"
#include <string>

/**
 * Temporary file paths for a single trial
 */
struct TrialTempFiles {
    std::string your_forward_output;
    std::string your_inverse_output;
    std::string bzip2_forward_output;
    std::string bzip2_inverse_output;
    
    /**
     * Clean up all temporary files
     */
    void cleanup() const;
};

/**
 * Helper class for running BWT benchmarks
 * Encapsulates timing and execution of BWT operations
 */
class BWTBenchmarkRunner {
public:
    /**
     * Run your forward BWT implementation and measure time
     * @param input_file Input file path
     * @param output_file Output file path
     * @param block_size Block size for processing
     * @param elapsed_time Output parameter for elapsed time in milliseconds
     * @param output_size Output parameter for output file size
     * @return true on success, false on failure
     */
    static bool run_your_forward_bwt(const std::string& input_file, const std::string& output_file,
                                      size_t block_size, double& elapsed_time, size_t& output_size);
    
    /**
     * Run your inverse BWT implementation and measure time
     * @param input_file Input file path
     * @param output_file Output file path
     * @param block_size Block size for processing
     * @param elapsed_time Output parameter for elapsed time in milliseconds
     * @return true on success, false on failure
     */
    static bool run_your_inverse_bwt(const std::string& input_file, const std::string& output_file,
                                      size_t block_size, double& elapsed_time);
    
    /**
     * Run bzip2 forward BWT extractor and measure time
     * @param input_file Input file path
     * @param output_file Output file path
     * @param block_size Block size for processing
     * @param elapsed_time Output parameter for elapsed time in milliseconds
     * @param output_size Output parameter for output file size
     * @return true on success, false on failure
     */
    static bool run_bzip2_forward_bwt(const std::string& input_file, const std::string& output_file,
                                       size_t block_size, double& elapsed_time, size_t& output_size);
    
    /**
     * Run bzip2 inverse BWT extractor and measure time
     * @param input_file Input file path
     * @param output_file Output file path
     * @param block_size Block size for processing
     * @param elapsed_time Output parameter for elapsed time in milliseconds
     * @return true on success, false on failure
     */
    static bool run_bzip2_inverse_bwt(const std::string& input_file, const std::string& output_file,
                                       size_t block_size, double& elapsed_time);
    
    /**
     * Run a single performance trial
     * @param input_file Input file path
     * @param block_size Block size for processing
     * @param temp_files Temporary file paths
     * @param trial_result Output parameter for trial results
     * @return true on success, false on failure
     */
    static bool run_single_trial(const std::string& input_file, size_t block_size,
                                  const TrialTempFiles& temp_files, TrialResult& trial_result);
};

#endif // BWT_BENCHMARK_RUNNER_HPP

