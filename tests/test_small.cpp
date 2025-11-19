#include <iostream>
#include <cassert>
#include <string>
#include "../src/bwt.hpp"
#include "../src/inverse_bwt.hpp"

// Helper function to run a test and report results
void run_test(const std::string& test_name, bool passed) {
    std::cout << "[" << (passed ? "PASS" : "FAIL") << "] " << test_name << std::endl;
    assert(passed);
}

// Test forward BWT on a simple string
void test_forward_basic() {
    std::string input = "banana";
    std::string result = bwt_forward(input);
    // Expected: "annb~aa" (last column of sorted rotations)
    // Let's verify it's the correct length and contains expected characters
    assert(result.length() == input.length() + 1); // +1 for delimiter
    assert(result.find('~') != std::string::npos); // Should contain delimiter
    run_test("Forward BWT: basic string", true);
}

// Test inverse BWT on a known BWT string
void test_inverse_basic() {
    // For "banana", the BWT should be reversible
    std::string input = "banana";
    std::string bwt_result = bwt_forward(input);
    std::string recovered = bwt_inverse(bwt_result);
    assert(recovered == input);
    run_test("Inverse BWT: basic string", true);
}

// Test round-trip: forward then inverse should return original
void test_round_trip() {
    std::string input = "hello";
    std::string bwt = bwt_forward(input);
    std::string recovered = bwt_inverse(bwt);
    assert(recovered == input);
    run_test("Round-trip: hello", true);
}

// Test round-trip with different string
void test_round_trip_2() {
    std::string input = "mississippi";
    std::string bwt = bwt_forward(input);
    std::string recovered = bwt_inverse(bwt);
    assert(recovered == input);
    run_test("Round-trip: mississippi", true);
}

// Test empty string
void test_empty_string() {
    std::string input = "";
    std::string bwt = bwt_forward(input);
    std::string recovered = bwt_inverse(bwt);
    assert(recovered == input);
    run_test("Round-trip: empty string", true);
}

// Test single character
void test_single_char() {
    std::string input = "a";
    std::string bwt = bwt_forward(input);
    std::string recovered = bwt_inverse(bwt);
    assert(recovered == input);
    run_test("Round-trip: single character", true);
}

// Test repeated characters
void test_repeated_chars() {
    std::string input = "aaaa";
    std::string bwt = bwt_forward(input);
    std::string recovered = bwt_inverse(bwt);
    assert(recovered == input);
    run_test("Round-trip: repeated characters", true);
}

// Test string with special characters
void test_special_chars() {
    std::string input = "a!b@c#";
    std::string bwt = bwt_forward(input);
    std::string recovered = bwt_inverse(bwt);
    assert(recovered == input);
    run_test("Round-trip: special characters", true);
}

// Test custom delimiter
void test_custom_delimiter() {
    std::string input = "test";
    char delimiter = '$';
    std::string bwt = bwt_forward(input, delimiter);
    std::string recovered = bwt_inverse(bwt, delimiter);
    assert(recovered == input);
    run_test("Round-trip: custom delimiter", true);
}

// Test longer string
void test_longer_string() {
    std::string input = "the quick brown fox jumps over the lazy dog";
    std::string bwt = bwt_forward(input);
    std::string recovered = bwt_inverse(bwt);
    assert(recovered == input);
    run_test("Round-trip: longer string", true);
}

// Test string with newlines
void test_with_newlines() {
    std::string input = "line1\nline2\nline3";
    std::string bwt = bwt_forward(input);
    std::string recovered = bwt_inverse(bwt);
    assert(recovered == input);
    run_test("Round-trip: string with newlines", true);
}

int main() {
    std::cout << "Running BWT tests...\n" << std::endl;
    
    test_forward_basic();
    test_inverse_basic();
    test_round_trip();
    test_round_trip_2();
    test_empty_string();
    test_single_char();
    test_repeated_chars();
    test_special_chars();
    test_custom_delimiter();
    test_longer_string();
    test_with_newlines();
    
    std::cout << "\nAll tests passed!" << std::endl;
    return 0;
}

