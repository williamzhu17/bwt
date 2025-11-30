#ifndef PERFORMANCE_COMPARISON_HPP
#define PERFORMANCE_COMPARISON_HPP

#include <string>
#include <vector>
#include <chrono>

/**
 * Results from a single performance trial
 */
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
    
    /**
     * Calculate round trip times and speedup metrics
     */
    void calculate_metrics();
};

/**
 * Aggregated comparison results with statistics across multiple trials
 */
class ComparisonResult {
public:
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
    
    /**
     * Calculate statistics from all trials
     */
    void calculate_statistics();
    
private:
    /**
     * Calculate mean of a vector of values
     */
    static double calculate_mean(const std::vector<double>& values);
    
    /**
     * Calculate standard deviation of a vector of values
     */
    static double calculate_stddev(const std::vector<double>& values, double mean);
    
    /**
     * Extract and calculate statistics for forward BWT
     */
    void calculate_forward_statistics();
    
    /**
     * Extract and calculate statistics for inverse BWT
     */
    void calculate_inverse_statistics();
    
    /**
     * Extract and calculate statistics for round trip
     */
    void calculate_roundtrip_statistics();
};

/**
 * Helper class for measuring execution time
 */
class Timer {
public:
    /**
     * Start the timer
     */
    void start();
    
    /**
     * Stop the timer and return elapsed time in milliseconds
     */
    double stop();
    
private:
    std::chrono::high_resolution_clock::time_point start_time;
    bool is_running = false;
};

/**
 * Helper class for printing comparison results
 */
class ComparisonPrinter {
public:
    /**
     * Print timing statistics for a single metric
     * @param label Label for the metric
     * @param mean Mean value
     * @param stddev Standard deviation
     * @param min Minimum value
     * @param max Maximum value
     * @param num_trials Number of trials
     */
    static void print_timing_stats(const std::string& label, double mean, double stddev,
                                   double min, double max, size_t num_trials);
    
    /**
     * Print comparison metrics (speedup, time difference)
     * @param label Label for the comparison
     * @param speedup_mean Mean speedup
     * @param speedup_stddev Standard deviation of speedup
     * @param your_time Your implementation time
     * @param bzip2_time bzip2 implementation time
     * @param num_trials Number of trials
     */
    static void print_comparison_metrics(const std::string& label, double speedup_mean,
                                         double speedup_stddev, double your_time,
                                         double bzip2_time, size_t num_trials);
    
    /**
     * Print full comparison results with all statistics
     * @param result Comparison result to print
     */
    static void print_comparison(const ComparisonResult& result);
    
    /**
     * Print summary line for aggregation
     * Format: SUMMARY|test_name|phase|your_time_mean|bzip2_time_mean|speedup|winner|faster_by_pct
     * @param result Comparison result
     * @param phase Phase name (forward, inverse, roundtrip)
     */
    static void print_summary_line(const ComparisonResult& result, const std::string& phase,
                                   double your_time, double bzip2_time, double speedup);

private:
    /**
     * Print header for comparison section
     */
    static void print_header(const ComparisonResult& result);
    
    /**
     * Print forward BWT section
     */
    static void print_forward_section(const ComparisonResult& result);
    
    /**
     * Print inverse BWT section
     */
    static void print_inverse_section(const ComparisonResult& result);
    
    /**
     * Print round trip section
     */
    static void print_roundtrip_section(const ComparisonResult& result);
    
    /**
     * Calculate and print throughput information
     */
    static void print_throughput(const ComparisonResult& result);
};

#endif // PERFORMANCE_COMPARISON_HPP

