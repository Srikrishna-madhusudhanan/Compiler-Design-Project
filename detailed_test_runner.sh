#!/bin/bash

# Enhanced test runner with categorization
cd /home/hades/chamber_of_secrets/Compiler-Design-Project

COMPILER="./build/parser"
TEST_DIR="./test"
RESULTS_FILE="detailed_test_results.txt"

# Files that SHOULD have errors (intentional test cases)
INTENTIONAL_ERROR_FILES=(
    "test/basics/invalid_switch.c"
    "test/basics/minimal_main.c"
    "test/basics/relop.c"
    "test/features/const_test.c"
    "test/features/diagnostic_missing_semicolon_regression.c"
    "test/features/diagnostic_typo_printf_regression.c"
)

# Files that should NOT exist or we know have unsupported features
KNOWN_UNSUPPORTED_FILES=(
    "test/oop/access_modifiers.c"
    "test/oop/bst.c"
    "test/oop/classes.c"
    "test/oop/constructors.c"
    "test/oop/method_overloading.c"
    "test/oop/polymorphism_demo.c"
    "test/oop/stack.c"
    "test/oop/virtual_methods.c"
)

# Clear previous results
> "$RESULTS_FILE"

echo "=====================================================" | tee -a "$RESULTS_FILE"
echo "DETAILED COMPILER TEST REPORT" | tee -a "$RESULTS_FILE"
echo "=====================================================" | tee -a "$RESULTS_FILE"
echo "" | tee -a "$RESULTS_FILE"

# Counters
total_tests=0
tests_passing=0
tests_intentional_error=0
tests_unsupported=0
tests_false_positive=0

echo "INTENTIONAL ERROR TESTS" | tee -a "$RESULTS_FILE"
echo "=====================================================  " | tee -a "$RESULTS_FILE"

for file in "${INTENTIONAL_ERROR_FILES[@]}"; do
    if [ -f "$file" ]; then
        ((total_tests++))
        echo "Testing: $file" | tee -a "$RESULTS_FILE"
        
        output=$("$COMPILER" "$file" 2>&1)
        
        # Check for errors
        has_error=0
        if echo "$output" | grep -q "Parsing Done with [1-9]"; then
            has_error=1
        fi
        if echo "$output" | grep -qi "semantic analysis failed"; then
            has_error=1
        fi
        
        if [ $has_error -eq 1 ]; then
            echo "  ✓ Correctly detected error" | tee -a "$RESULTS_FILE"
            ((tests_intentional_error++))
            # Extract error message
            error_msg=$(echo "$output" | grep -i "error" | head -1)
            echo "    Error: $error_msg" | tee -a "$RESULTS_FILE"
        else
            echo "  ✗ FAILED: Expected error not detected" | tee -a "$RESULTS_FILE"
        fi
        echo "" | tee -a "$RESULTS_FILE"
    fi
done

echo "" | tee -a "$RESULTS_FILE"
echo "KNOWN UNSUPPORTED/OOP TESTS" | tee -a "$RESULTS_FILE"
echo "====================================================" | tee -a "$RESULTS_FILE"

for file in "${KNOWN_UNSUPPORTED_FILES[@]}"; do
    if [ -f "$file" ]; then
        ((total_tests++))
        echo "Testing: $file (OOP feature)" | tee -a "$RESULTS_FILE"
        
        output=$("$COMPILER" "$file" 2>&1)
        
        # Check for misleading hints
        if echo "$output" | grep -i "looks similar to keyword"; then
            echo "  ⚠ ISSUE: False positive hint detected" | tee -a "$RESULTS_FILE"
            hint=$(echo "$output" | grep -i "looks similar" | head -1)
            echo "    False Hint: $hint" | tee -a "$RESULTS_FILE"
            ((tests_false_positive++))
        else
            echo "  ℹ Error (expected for unsupported OOP)" | tee -a "$RESULTS_FILE"
            ((tests_unsupported++))
        fi
        echo "" | tee -a "$RESULTS_FILE"
    fi
done

echo "" | tee -a "$RESULTS_FILE"
echo "REGULAR TEST FILES (SHOULD PASS)" | tee -a "$RESULTS_FILE"
echo "====================================================" | tee -a "$RESULTS_FILE"

regular_pass=0
regular_fail=0

find "$TEST_DIR" -name "*.c" -type f | sort | while read file; do
    # Skip intentional error files and unsupported files
    skip=0
    for skip_file in "${INTENTIONAL_ERROR_FILES[@]}" "${KNOWN_UNSUPPORTED_FILES[@]}"; do
        if [ "$file" = "$skip_file" ]; then
            skip=1
            break
        fi
    done
    
    if [ $skip -eq 1 ]; then
        continue
    fi
    
    ((total_tests++))
    output=$("$COMPILER" "$file" 2>&1)
    
    # Check for errors
    has_error=0
    if echo "$output" | grep -q "Parsing Done with [1-9]"; then
        has_error=1
    fi
    if echo "$output" | grep -qi "semantic analysis failed"; then
        has_error=1
    fi
    
    if [ $has_error -eq 0 ]; then
        echo "  ✓ $file"
        ((regular_pass++))
    else
        echo "  ✗ $file (unexpected error)" | tee -a "$RESULTS_FILE"
        ((regular_fail++))
    fi
done | tee -a "$RESULTS_FILE"

echo "" | tee -a "$RESULTS_FILE"
echo "=====================================================" | tee -a "$RESULTS_FILE"
echo "SUMMARY" | tee -a "$RESULTS_FILE"
echo "=====================================================" | tee -a "$RESULTS_FILE"
echo "Total Unique Test Files: $total_tests" | tee -a "$RESULTS_FILE"
echo "Intentional Error Tests (✓): $tests_intentional_error" | tee -a "$RESULTS_FILE"
echo "Unsupported OOP Files: $tests_unsupported" | tee -a "$RESULTS_FILE"
echo "FALSE POSITIVES (with misleading hints): $tests_false_positive" | tee -a "$RESULTS_FILE"
echo "Regular Tests Passing: $regular_pass" | tee -a "$RESULTS_FILE"
echo "Regular Tests Failing: $regular_fail" | tee -a "$RESULTS_FILE"
echo "=====================================================" | tee -a "$RESULTS_FILE"
