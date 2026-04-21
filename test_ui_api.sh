#!/bin/bash

# Test the GUI API endpoints programmatically
cd /home/hades/chamber_of_secrets/Compiler-Design-Project

BASE_URL="http://127.0.0.1:3000"
RESULTS_FILE="ui_test_results.txt"

> "$RESULTS_FILE"

echo "=====================================================" | tee -a "$RESULTS_FILE"
echo "GUI API TEST RESULTS" | tee -a "$RESULTS_FILE"
echo "=====================================================" | tee -a "$RESULTS_FILE"
echo "" | tee -a "$RESULTS_FILE"

# Function to test compilation
test_compile() {
    local test_file=$1
    local opt_level=$2
    local test_name=$3
    
    echo "Testing Compile & Viz: $test_name (O$opt_level)" | tee -a "$RESULTS_FILE"
    
    code=$(cat "$test_file" 2>/dev/null)
    if [ -z "$code" ]; then
        echo "  ✗ FAILED: Could not read file" | tee -a "$RESULTS_FILE"
        return 1
    fi
    
    # Create JSON payload
    payload=$(cat <<EOF
{
    "code": $(echo "$code" | jq -Rs '.'),
    "optimizationLevel": $opt_level,
    "useMetrics": true
}
EOF
)
    
    # Make request (with 5 second timeout)
    response=$(timeout 5 curl -s -X POST "$BASE_URL/api/compile" \
        -H "Content-Type: application/json" \
        -d "$payload" 2>&1)
    
    if [ -z "$response" ]; then
        echo "  ✗ FAILED: No response from server (timeout)" | tee -a "$RESULTS_FILE"
        return 1
    fi
    
    # Check if response contains expected fields
    if echo "$response" | jq empty 2>/dev/null; then
        if echo "$response" | jq -e '.stdout' >/dev/null 2>&1; then
            echo "  ✓ PASSED: Received valid response" | tee -a "$RESULTS_FILE"
            
            # Check for errors in stderr
            stderr=$(echo "$response" | jq -r '.stderr' 2>/dev/null)
            if [ ! -z "$stderr" ] && [ "$stderr" != "null" ]; then
                # Check if the error is expected
                if echo "$stderr" | grep -q "Semantic analysis failed\|Parser Error"; then
                    echo "    ℹ Compiler error detected (check if intentional)" | tee -a "$RESULTS_FILE"
                fi
            fi
            
            # Check for IR output
            if echo "$response" | jq -e '.ir' >/dev/null 2>&1; then
                ir_len=$(echo "$response" | jq '.ir | length' 2>/dev/null)
                echo "    + IR output: $ir_len bytes" | tee -a "$RESULTS_FILE"
            fi
            
            # Check for metrics
            if echo "$response" | jq -e '.metrics' >/dev/null 2>&1; then
                metrics=$(echo "$response" | jq '.metrics' 2>/dev/null)
                if [ ! -z "$metrics" ] && [ "$metrics" != "null" ]; then
                    echo "    + Metrics collected" | tee -a "$RESULTS_FILE"
                fi
            fi
            
            return 0
        fi
    fi
    
    echo "  ✗ FAILED: Invalid or malformed response" | tee -a "$RESULTS_FILE"
    return 1
}

# Test a few representative files via the UI API
echo "COMPILE & VIZ TESTS" | tee -a "$RESULTS_FILE"
echo "=====================================================" | tee -a "$RESULTS_FILE"

test_compile "test/basics/simple_add.c" 0 "Simple Addition (O0)"
echo "" | tee -a "$RESULTS_FILE"

test_compile "test/basics/simple_add.c" 1 "Simple Addition (O1)"
echo "" | tee -a "$RESULTS_FILE"

test_compile "test/complex/factorial_recursive.c" 2 "Recursive Factorial (O2)"
echo "" | tee -a "$RESULTS_FILE"

test_compile "test/features/diagnostic_missing_semicolon_regression.c" 0 "Diagnostic: Missing Semicolon"
echo "" | tee -a "$RESULTS_FILE"

test_compile "test/features/diagnostic_typo_printf_regression.c" 0 "Diagnostic: Printf Typo"
echo "" | tee -a "$RESULTS_FILE"

test_compile "test/oop/access_modifiers.c" 0 "OOP: Access Modifiers (should show false hint)"
echo "" | tee -a "$RESULTS_FILE"

echo "=====================================================" | tee -a "$RESULTS_FILE"
echo "UI API TESTING COMPLETE" | tee -a "$RESULTS_FILE"
echo "=====================================================" | tee -a "$RESULTS_FILE"

cat "$RESULTS_FILE"
