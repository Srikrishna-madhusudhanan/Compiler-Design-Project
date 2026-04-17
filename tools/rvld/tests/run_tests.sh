#!/bin/bash

# Absolute paths
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
ROOT_DIR="$( cd "$SCRIPT_DIR/../../../" && pwd )"
RVAS="$ROOT_DIR/tools/rvas/rvas"
RVLD="$ROOT_DIR/tools/rvld/rvld"
CRT0="$SCRIPT_DIR/crt0.o"

# Build toolchain first from root
echo "Building Toolchain..."
make -C "$ROOT_DIR" toolchain > /dev/null

echo "Starting Linker Verification Tests..."
echo "------------------------------------"

failed=0
passed=0

check_result() {
    name=$1
    binary=$2
    expected=$3
    
    if [ ! -f "$binary" ]; then
        echo "FAIL (Binary not found: $binary)"
        failed=$((failed+1))
        return
    fi
    
    chmod +x "$binary"
    qemu-riscv64 "$binary"
    actual=$?
    
    if [ $actual -eq $expected ]; then
        echo "PASS (Exit: $actual)"
        passed=$((passed+1))
    else
        echo "FAIL (Expected: $expected, Actual: $actual)"
        failed=$((failed+1))
    fi
}

# Ensure crt0 is built
"$RVAS" -o "$CRT0" "$SCRIPT_DIR/crt0.s"

# 1. test1
echo -n "Running test1... "
"$RVAS" -o "$SCRIPT_DIR/test1.o" "$SCRIPT_DIR/test1.s" && \
"$RVLD" -o "$SCRIPT_DIR/test1_exec" "$SCRIPT_DIR/test1.o" "$CRT0"
check_result "test1" "$SCRIPT_DIR/test1_exec" 42

# 2. test_multi
echo -n "Running test_multi... "
"$RVAS" -o "$SCRIPT_DIR/main.o" "$SCRIPT_DIR/main.s" && \
"$RVAS" -o "$SCRIPT_DIR/foo.o" "$SCRIPT_DIR/foo.s" && \
"$RVLD" -o "$SCRIPT_DIR/test_multi_exec" "$SCRIPT_DIR/main.o" "$SCRIPT_DIR/foo.o" "$CRT0"
check_result "test_multi" "$SCRIPT_DIR/test_multi_exec" 10

# 3. control_flow
echo -n "Running control_flow... "
"$RVAS" -o "$SCRIPT_DIR/control_flow.o" "$SCRIPT_DIR/control_flow.s" && \
"$RVLD" -o "$SCRIPT_DIR/control_flow_exec" "$SCRIPT_DIR/control_flow.o" "$CRT0"
check_result "control_flow" "$SCRIPT_DIR/control_flow_exec" 24

# 4. bubble_sort
echo -n "Running bubble_sort... "
"$RVAS" -o "$SCRIPT_DIR/bubble_sort.o" "$SCRIPT_DIR/bubble_sort.s" && \
"$RVLD" -o "$SCRIPT_DIR/bubble_sort_exec" "$SCRIPT_DIR/bubble_sort.o" "$CRT0"
check_result "bubble_sort" "$SCRIPT_DIR/bubble_sort_exec" 0

# 5. glob_test
echo -n "Running glob_test... "
"$RVAS" -o "$SCRIPT_DIR/glob_main.o" "$SCRIPT_DIR/glob_main.s" && \
"$RVAS" -o "$SCRIPT_DIR/glob_data.o" "$SCRIPT_DIR/glob_data.s" && \
"$RVLD" -o "$SCRIPT_DIR/glob_test_exec" "$SCRIPT_DIR/glob_main.o" "$SCRIPT_DIR/glob_data.o" "$CRT0"
check_result "glob_test" "$SCRIPT_DIR/glob_test_exec" 123

# 6. poly_fixed
echo -n "Running poly_fixed... "
"$RVAS" -o "$SCRIPT_DIR/poly_fixed.o" "$SCRIPT_DIR/poly_fixed.s" && \
"$RVLD" -o "$SCRIPT_DIR/poly_fixed_exec" "$SCRIPT_DIR/poly_fixed.o"
check_result "poly_fixed" "$SCRIPT_DIR/poly_fixed_exec" 2

# 7. loop_unroll
echo -n "Running loop_unroll... "
"$RVAS" -o "$SCRIPT_DIR/loop_unroll.o" "$SCRIPT_DIR/loop_unroll.s" && \
"$RVLD" -o "$SCRIPT_DIR/loop_unroll_exec" "$SCRIPT_DIR/loop_unroll.o" "$CRT0"
check_result "loop_unroll" "$SCRIPT_DIR/loop_unroll_exec" 45


echo "------------------------------------"
echo "Results: $passed Passed, $failed Failed"

if [ $failed -eq 0 ]; then
    exit 0
else
    exit 1
fi
