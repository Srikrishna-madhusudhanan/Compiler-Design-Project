#!/bin/bash
set -euo pipefail

cd "$(dirname "$0")/.."
PARSER=./build/parser

make -q parser 2>/dev/null || make parser

pass=0
fail=0

check_contains() {
    local text="$1"
    local expected="$2"
    if echo "$text" | grep -Fq "$expected"; then
        pass=$((pass+1))
    else
        echo "[FAIL] Expected output to contain: $expected"
        echo "$text"
        fail=$((fail+1))
    fi
}

# 1) typedef alias regression should compile semantically
out=$($PARSER test/features/typedef_alias_regression.c 2>&1 || true)
check_contains "$out" "Semantic analysis successful."

# 2) missing semicolon should produce parser hint
out=$($PARSER test/features/diagnostic_missing_semicolon_regression.c 2>&1 || true)
check_contains "$out" "Parser Error:"
check_contains "$out" "Hint: it looks like you may have forgotten a semicolon"

# 3) typo suggestion should propose printf
out=$($PARSER test/features/diagnostic_typo_printf_regression.c 2>&1 || true)
check_contains "$out" "Did you mean 'printf'?"

echo "Feature regressions Passed: $pass  Failed: $fail"
[ $fail -eq 0 ]
