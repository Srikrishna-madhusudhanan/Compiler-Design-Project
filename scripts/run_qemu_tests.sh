#!/bin/bash
set -euo pipefail

ROOT_DIR=$(cd "$(dirname "$0")/.." && pwd)
cd "$ROOT_DIR"

TEST_HELPER=./scripts/qemu_run.sh
PARSER=./build/parser
if [ ! -x "$TEST_HELPER" ]; then
  chmod +x "$TEST_HELPER" 2>/dev/null || true
fi

if [ ! -x "$PARSER" ]; then
  make setup parser >/dev/null
fi

if [ ! -f "$TEST_HELPER" ]; then
  echo "Error: helper script '$TEST_HELPER' not found." >&2
  exit 1
fi

if [ $# -gt 0 ]; then
  case "$1" in
    --list)
      echo "factorial_tail_recursive"
      exit 0
      ;;
    *)
      echo "Usage: $0 [--list]" >&2
      exit 1
      ;;
  esac
fi

PASS=0
FAIL=0

run_test() {
  local src="$1"
  local input="$2"
  local expected="$3"
  local name="$4"
  local flags="${5:-}"

  printf "Running %s... " "$name"
  local actual
  local -a cmd=("bash" "$TEST_HELPER")
  if [ -n "$flags" ]; then
    # shellcheck disable=SC2206
    local flag_arr=($flags)
    cmd+=("${flag_arr[@]}")
  fi
  cmd+=("$src" "$input")

  if ! actual=$("${cmd[@]}" 2>/dev/null); then
    echo "FAIL (execution error)"
    FAIL=$((FAIL + 1))
    return
  fi
  actual=$(printf "%s" "$actual" | sed '/^-->/d')
  actual=$(printf "%s" "$actual" | sed '/^$/d')

  if [ "$actual" = "$expected" ]; then
    echo "PASS"
    PASS=$((PASS + 1))
  else
    echo "FAIL"
    echo "Expected: $expected"
    echo "Actual:   $actual"
    FAIL=$((FAIL + 1))
  fi
}

run_ir_no_backedge_test() {
  local src="$1"
  local name="$2"

  printf "Running %s... " "$name"

  if ! "$PARSER" -O2 "$src" >/dev/null 2>&1; then
    echo "FAIL (parser error)"
    FAIL=$((FAIL + 1))
    return
  fi

  if [ ! -f "ir_opt.txt" ]; then
    echo "FAIL (missing ir_opt.txt)"
    FAIL=$((FAIL + 1))
    return
  fi

  # A backward branch (goto/if target label appearing earlier in the IR file)
  # indicates a potential loop back-edge after optimization.
  if awk '
      /^[A-Za-z_][A-Za-z0-9_]*:/ {
        lbl=$1
        sub(/:$/, "", lbl)
        label_line[lbl]=NR
      }
      {
        p=index($0, "goto ")
        if (p > 0) {
          target=substr($0, p + 5)
          sub(/^[[:space:]]+/, "", target)
          split(target, parts, /[^A-Za-z0-9_]/)
          if (parts[1] != "") {
            jump_line[++jump_count]=NR
            jump_target[jump_count]=parts[1]
          }
        }
      }
      END {
        bad=0
        for (i=1; i<=jump_count; i++) {
          t=jump_target[i]
          if ((t in label_line) && label_line[t] < jump_line[i]) {
            bad=1
          }
        }
        exit bad
      }
    ' ir_opt.txt; then
    echo "PASS"
    PASS=$((PASS + 1))
  else
    echo "FAIL"
    echo "Found backward branch(es) in optimized IR (possible residual loop back-edge)."
    FAIL=$((FAIL + 1))
  fi
}

# Regression test for tail-recursive factorial bug.
# The program should compute 7! = 5040.
run_test "test/complex/factorial_tail_recursive.c" "7\n" "Enter a positive integer: Tail-Recursive Factorial is 5040" "factorial_tail_recursive"

# Full-unroll regression: multi-block loop body with exact trip count.
run_test "test/optimizations/full_unroll_multiblock.c" "" "sum=24" "full_unroll_multiblock" "-O2"

# Full-unroll regression: trip count above historical capped behavior.
run_test "test/optimizations/full_unroll_large_trip.c" "" "total=160" "full_unroll_large_trip" "-O2"

# IR-structure assertions: fully unrolled cases should have no backward branches.
run_ir_no_backedge_test "test/optimizations/full_unroll_multiblock.c" "ir_no_backedge_full_unroll_multiblock"
run_ir_no_backedge_test "test/optimizations/full_unroll_large_trip.c" "ir_no_backedge_full_unroll_large_trip"

printf "\nResults: Passed=%d  Failed=%d\n" "$PASS" "$FAIL"
[ "$FAIL" -eq 0 ]
