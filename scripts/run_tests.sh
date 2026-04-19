#!/bin/bash
# Week 4: IR - parse a file and get its IR, or run validation tests

cd "$(dirname "$0")/.."
PARSER=./build/parser

make -q parser 2>/dev/null || make parser

if [ -n "$1" ]; then
    # Parse single file, show IR (saved to ir.txt)
    $PARSER "$1"
else
    # Batch validation: pass/fail only (no ir.txt left behind)
    rm -f ir.txt
    PASS=0 FAIL=0

    run_section() {
        local title="$1"
        echo
        echo "=== [$title] ==="
    }

    run_expect_success() {
        local case_name="$1"
        local file="$2"
        local out
        if out=$($PARSER "$file" 2>&1); then
            echo "[PASS] $case_name"
            PASS=$((PASS+1))
        else
            echo "[FAIL] $case_name"
            echo "  expected: parse+semantic success"
            echo "  file: $file"
            echo "$out"
            FAIL=$((FAIL+1))
        fi
    }

    run_expect_parser_error_with_hint() {
        local case_name="$1"
        local file="$2"
        local hint="$3"
        local out
        out=$($PARSER "$file" 2>&1)
        if echo "$out" | grep -Fq "Parser Error:" && echo "$out" | grep -Fq "$hint"; then
            echo "[PASS] $case_name"
            PASS=$((PASS+1))
        else
            echo "[FAIL] $case_name"
            echo "  expected: parser error containing hint"
            echo "  file: $file"
            echo "  hint: $hint"
            echo "$out"
            FAIL=$((FAIL+1))
        fi
    }

    run_expect_output_contains() {
        local case_name="$1"
        local file="$2"
        local text="$3"
        local out
        if out=$($PARSER "$file" 2>&1); then
            :
        fi
        if echo "$out" | grep -Fq "$text"; then
            echo "[PASS] $case_name"
            PASS=$((PASS+1))
        else
            echo "[FAIL] $case_name"
            echo "  expected output to contain: $text"
            echo "  file: $file"
            echo "$out"
            FAIL=$((FAIL+1))
        fi
    }

    run_section "LEGACY"
    run_expect_success "legacy.simple_add" "test/basics/simple_add.c"
    run_expect_success "legacy.switch_stmt" "test/features/switch_stmt.c"
    run_expect_success "legacy.classes" "test/oop/classes.c"
    run_expect_success "legacy.virtual_methods" "test/oop/virtual_methods.c"

    run_section "CORE"
    run_expect_success "core.minimal_main" "test/basics/minimal_main.c"
    run_expect_success "core.variable_init" "test/basics/variable_init.c"
    run_expect_success "core.control_flow" "test/features/control_flow.c"
    run_expect_success "core.function_calls" "test/features/function_calls.c"
    run_expect_output_contains "core.invalid_switch_negative" "test/basics/invalid_switch.c" "Semantic analysis failed"

    run_section "FEATURES"
    run_expect_success "features.structs" "test/features/structs.c"
    run_expect_success "features.struct_pointers" "test/features/struct_pointers.c"
    run_expect_success "features.typedef_alias_regression" "test/features/typedef_alias_regression.c"

    run_section "DIAGNOSTICS"
    run_expect_parser_error_with_hint "diagnostics.missing_semicolon_hint" "test/features/diagnostic_missing_semicolon_regression.c" "Hint: it looks like you may have forgotten a semicolon"
    run_expect_output_contains "diagnostics.typo_printf_hint" "test/features/diagnostic_typo_printf_regression.c" "Did you mean 'printf'?"

    TOTAL=$((PASS + FAIL))
    STATUS="pass"
    if [ $FAIL -ne 0 ]; then
        STATUS="fail"
    fi

    echo "Passed: $PASS  Failed: $FAIL"
    printf '{"suite":"run_tests","status":"%s","passed":%d,"failed":%d,"total":%d}\n' "$STATUS" "$PASS" "$FAIL" "$TOTAL"

    [ $FAIL -eq 0 ] && exit 0 || exit 1
fi
