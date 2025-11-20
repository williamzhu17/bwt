#include <iostream>
#include <cassert>
#include <string>
#include <vector>
#include <functional>
#include "../src/bwt.hpp"
#include "../src/inverse_bwt.hpp"

// Test result structure
struct TestResult {
    bool passed;
    std::string error_msg;
    
    TestResult(bool p, const std::string& msg = "") : passed(p), error_msg(msg) {}
};

// Test case structure
struct TestCase {
    std::string name;
    std::function<TestResult()> test_func;
};

// Helper function to run a test and report results
void run_test(const std::string& test_name, const TestResult& result) {
    std::cout << "[" << (result.passed ? "PASS" : "FAIL") << "] " << test_name;
    if (!result.passed && !result.error_msg.empty()) {
        std::cout << " - " << result.error_msg;
    }
    std::cout << std::endl;
}

// Test forward BWT on a simple string
TestResult test_forward_basic() {
    std::string input = "banana";
    std::string result = bwt_forward(input);
    // Expected: "annb~aa" (last column of sorted rotations)
    
    if (result.length() != input.length() + 1) {
        return TestResult(false, "Expected length " + std::to_string(input.length() + 1) + 
                          ", got " + std::to_string(result.length()));
    }
    if (result.find('~') == std::string::npos) {
        return TestResult(false, "Result does not contain delimiter '~'");
    }
    if (result != "annb~aa") {
        return TestResult(false, "Expected \"annb~aa\", got \"" + result + "\"");
    }
    return TestResult(true);
}

// Test inverse BWT on a known BWT string
TestResult test_inverse_basic() {
    // For "banana", the BWT should be reversible
    std::string input = "banana";
    std::string bwt_result = bwt_forward(input);
    std::string recovered = bwt_inverse(bwt_result);
    if (recovered != input) {
        return TestResult(false, "Expected \"" + input + "\", got \"" + recovered + "\"");
    }
    return TestResult(true);
}

// Test round-trip: forward then inverse should return original
TestResult test_round_trip() {
    std::string input = "hello";
    std::string bwt = bwt_forward(input);
    std::string recovered = bwt_inverse(bwt);
    if (recovered != input) {
        return TestResult(false, "Expected \"" + input + "\", got \"" + recovered + "\"");
    }
    return TestResult(true);
}

// Test round-trip with different string
TestResult test_round_trip_2() {
    std::string input = "mississippi";
    std::string bwt = bwt_forward(input);
    std::string recovered = bwt_inverse(bwt);
    if (recovered != input) {
        return TestResult(false, "Expected \"" + input + "\", got \"" + recovered + "\"");
    }
    return TestResult(true);
}

// Test empty string
TestResult test_empty_string() {
    std::string input = "";
    std::string bwt = bwt_forward(input);
    std::string recovered = bwt_inverse(bwt);
    if (recovered != input) {
        return TestResult(false, "Expected empty string, got \"" + recovered + "\"");
    }
    return TestResult(true);
}

// Test single character
TestResult test_single_char() {
    std::string input = "a";
    std::string bwt = bwt_forward(input);
    std::string recovered = bwt_inverse(bwt);
    if (recovered != input) {
        return TestResult(false, "Expected \"" + input + "\", got \"" + recovered + "\"");
    }
    return TestResult(true);
}

// Test repeated characters
TestResult test_repeated_chars() {
    std::string input = "aaaa";
    std::string bwt = bwt_forward(input);
    std::string recovered = bwt_inverse(bwt);
    if (recovered != input) {
        return TestResult(false, "Expected \"" + input + "\", got \"" + recovered + "\"");
    }
    return TestResult(true);
}

// Test string with special characters
TestResult test_special_chars() {
    std::string input = "a!b@c#";
    std::string bwt = bwt_forward(input);
    std::string recovered = bwt_inverse(bwt);
    if (recovered != input) {
        return TestResult(false, "Expected \"" + input + "\", got \"" + recovered + "\"");
    }
    return TestResult(true);
}

// Test custom delimiter
TestResult test_custom_delimiter() {
    std::string input = "test";
    char delimiter = '$';
    std::string bwt = bwt_forward(input, delimiter);
    std::string recovered = bwt_inverse(bwt, delimiter);
    if (recovered != input) {
        return TestResult(false, "Expected \"" + input + "\", got \"" + recovered + "\"");
    }
    return TestResult(true);
}

// Test longer string
TestResult test_longer_string() {
    std::string input = "the quick brown fox jumps over the lazy dog";
    std::string bwt = bwt_forward(input);
    std::string recovered = bwt_inverse(bwt);
    if (recovered != input) {
        return TestResult(false, "Round-trip failed: recovered string does not match input");
    }
    return TestResult(true);
}

// Test string with newlines
TestResult test_with_newlines() {
    std::string input = "line1\nline2\nline3";
    std::string bwt = bwt_forward(input);
    std::string recovered = bwt_inverse(bwt);
    if (recovered != input) {
        return TestResult(false, "Round-trip failed: recovered string does not match input");
    }
    return TestResult(true);
}

int main() {
    std::cout << "Running BWT tests...\n" << std::endl;
    
    // Register all tests with their names
    std::vector<TestCase> tests = {
        {"Forward BWT: basic string", test_forward_basic},
        {"Inverse BWT: basic string", test_inverse_basic},
        {"Round-trip: hello", test_round_trip},
        {"Round-trip: mississippi", test_round_trip_2},
        {"Round-trip: empty string", test_empty_string},
        {"Round-trip: single character", test_single_char},
        {"Round-trip: repeated characters", test_repeated_chars},
        {"Round-trip: special characters", test_special_chars},
        {"Round-trip: custom delimiter", test_custom_delimiter},
        {"Round-trip: longer string", test_longer_string},
        {"Round-trip: string with newlines", test_with_newlines}
    };
    
    // Run all tests and track results
    int passed_count = 0;
    int failed_count = 0;
    
    for (const auto& test_case : tests) {
        TestResult result = test_case.test_func();
        run_test(test_case.name, result);
        if (result.passed) {
            passed_count++;
        } else {
            failed_count++;
        }
    }
    
    // Print summary
    std::cout << "\n=== Test Summary ===" << std::endl;
    std::cout << "Total tests: " << tests.size() << std::endl;
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
