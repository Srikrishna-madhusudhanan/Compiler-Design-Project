#!/bin/bash

# Test runner script - Tests all test cases via terminal
# Logs results to a file for analysis

cd /home/hades/chamber_of_secrets/Compiler-Design-Project

COMPILER="./build/parser"
TEST_DIR="./test"
RESULTS_FILE="test_results.txt"
ERROR_FILES=("test/features/diagnostic_missing_semicolon_regression.c" "test/features/diagnostic_typo_printf_regression.c")

# Clear previous results
> "$RESULTS_FILE"

echo "=====================================================" | tee -a "$RESULTS_FILE"
echo "COMPILER TEST RUNNER - Terminal Output Tests" | tee -a "$RESULTS_FILE"
echo "=====================================================" | tee -a "$RESULTS_FILE"
echo "" | tee -a "$RESULTS_FILE"

# Find all .c files in test directory
find_test_files() {
    find "$TEST_DIR" -name "*.c" -type f | sort
}

should_have_error() {
    local file="$1"
    for error_file in "${ERROR_FILES[@]}"; do
        if [ "$file" = "$error_file" ]; then
            return 0  # Should have error
        fi
    done
    return 1  # Should NOT have error
}

# Counter variables
total_tests=0
passed_tests=0
failed_tests=0

# Test each file
for test_file in $(find_test_files); do
    ((total_tests++))
    relative_path="${test_file#./}"
    
    echo "Testing: $relative_path" | tee -a "$RESULTS_FILE"
    
    # Run compiler and capture output
    output=$("$COMPILER" "$test_file" 2>&1)
    exit_code=$?
    
    # Check for actual parser/semantic errors in output
    # Look for "Parsing Done with X errors" where X > 0, or "error" at start of a line
    has_error=0
    
    # Check if parser reported errors
    if echo "$output" | grep -q "Parsing Done with [1-9]"; then
        has_error=1
    fi
    
    # Check for semantic analysis failure
    if echo "$output" | grep -qi "semantic analysis failed"; then
        has_error=1
    fi
    
    # Check for lines that start with "error" (case insensitive)
    if echo "$output" | grep -i "^error"; then
        has_error=1
    fi
    
    # Check if this file should have an error
    if should_have_error "$test_file"; then
        if [ $has_error -eq 1 ]; then
            echo "  ✓ EXPECTED ERROR (correctly detected)" | tee -a "$RESULTS_FILE"
            ((passed_tests++))
        else
            echo "  ✗ FAILED: Expected error but compilation succeeded" | tee -a "$RESULTS_FILE"
            echo "    Output: $output" | tee -a "$RESULTS_FILE"
            ((failed_tests++))
        fi
    else
        # File should NOT have an error
        if [ $has_error -eq 1 ]; then
            echo "  ✗ FAILED: Unexpected error found" | tee -a "$RESULTS_FILE"
            echo "    Output: $output" | tee -a "$RESULTS_FILE"
            ((failed_tests++))
        else
            echo "  ✓ PASSED: No errors (as expected)" | tee -a "$RESULTS_FILE"
            ((passed_tests++))
        fi
    fi
    echo "" | tee -a "$RESULTS_FILE"
done

# Summary
echo "=====================================================" | tee -a "$RESULTS_FILE"
echo "TEST SUMMARY" | tee -a "$RESULTS_FILE"
echo "=====================================================" | tee -a "$RESULTS_FILE"
echo "Total Tests: $total_tests" | tee -a "$RESULTS_FILE"
echo "Passed:      $passed_tests" | tee -a "$RESULTS_FILE"
echo "Failed:      $failed_tests" | tee -a "$RESULTS_FILE"
echo "=====================================================" | tee -a "$RESULTS_FILE"
