#include <iostream>
#include <cassert>
#include <string>
#include <vector>
#include "../src/bwt.hpp"
#include "../src/inverse_bwt.hpp"

// Test result structure
struct TestResult {
    bool passed;
    std::string error_msg;
    
    TestResult(bool p, const std::string& msg = "") : passed(p), error_msg(msg) {}
};

// Test case structure containing input data
struct TestCase {
    std::string name;
    std::string input;
    char delimiter;
    std::string expected_forward;  // Empty string means don't check forward result
};

// Helper function to create a test case with named parameters for clarity
TestCase make_test_case(const std::string& name, const std::string& input, 
                       char delimiter = '~', const std::string& expected_forward = "") {
    return {name, input, delimiter, expected_forward};
}

// Helper function to run a test and report results
void run_test(const std::string& test_name, const TestResult& result) {
    std::cout << "[" << (result.passed ? "PASS" : "FAIL") << "] " << test_name;
    if (!result.passed && !result.error_msg.empty()) {
        std::cout << " - " << result.error_msg;
    }
    std::cout << std::endl;
}

// Generic test function for BWT round-trip with optional forward result check
// Takes input string, delimiter, and optionally expected forward result
TestResult test_bwt_round_trip(const std::string& input, char delimiter = '~', 
                                 const std::string& expected_forward = "") {
    // Run forward BWT
    std::string forward_result = bwt_forward(input, delimiter);
    
    // Check length
    if (forward_result.length() != input.length() + 1) {
        return TestResult(false, "Forward result length mismatch: expected " + 
                          std::to_string(input.length() + 1) + ", got " + 
                          std::to_string(forward_result.length()));
    }
    
    // Check delimiter is present
    if (forward_result.find(delimiter) == std::string::npos) {
        return TestResult(false, "Forward result does not contain delimiter '" + 
                          std::string(1, delimiter) + "'");
    }
    
    // Check expected forward result if provided
    if (!expected_forward.empty() && forward_result != expected_forward) {
        return TestResult(false, "Forward result mismatch: expected \"" + expected_forward + 
                          "\", got \"" + forward_result + "\"");
    }
    
    // Run inverse BWT and check round-trip
    std::string recovered = bwt_inverse(forward_result, delimiter);
    if (recovered != input) {
        return TestResult(false, "Round-trip failed: expected \"" + input + 
                          "\", got \"" + recovered + "\"");
    }
    
    return TestResult(true);
}

int main() {
    std::cout << "Running BWT tests...\n" << std::endl;
    
    // Define all test cases with their input data
    std::vector<TestCase> test_cases = {
        make_test_case("Forward BWT: basic string (with known result)", "banana", '~', "bnn~aaa"),
        make_test_case("Round-trip: hello", "hello"),
        make_test_case("Round-trip: mississippi", "mississippi"),
        make_test_case("Round-trip: empty string", ""),
        make_test_case("Round-trip: single character", "a"),
        make_test_case("Round-trip: repeated characters", "aaaa"),
        make_test_case("Round-trip: special characters", "a!b@c#"),
        make_test_case("Round-trip: custom delimiter", "test", '$'),
        make_test_case("Round-trip: longer string", "the quick brown fox jumps over the lazy dog"),
        make_test_case("Round-trip: string with newlines", "line1\nline2\nline3")
    };
    
    // Run all tests and track results
    int passed_count = 0;
    int failed_count = 0;
    
    for (const auto& test_case : test_cases) {
        TestResult result = test_bwt_round_trip(test_case.input, test_case.delimiter, 
                                                test_case.expected_forward);
        run_test(test_case.name, result);
        if (result.passed) {
            passed_count++;
        } else {
            failed_count++;
        }
    }
    
    // Print summary
    std::cout << "\n=== Test Summary ===" << std::endl;
    std::cout << "Total tests: " << test_cases.size() << std::endl;
    std::cout << "Passed: " << passed_count << std::endl;
    std::cout << "Failed: " << failed_count << std::endl;
    
    if (failed_count == 0) {
        std::cout << "\nAll tests passed!" << std::endl;
        return 0;
    } else {
        std::cout << "\nSome tests failed!" << std::endl;
        return 1;
    }
}
