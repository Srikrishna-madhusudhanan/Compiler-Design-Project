#!/bin/bash
set -euo pipefail

ROOT_DIR=$(cd "$(dirname "$0")/.." && pwd)
cd "$ROOT_DIR"

PARSER=./build/parser
if [ ! -x "$PARSER" ]; then
  make parser
fi

RISCVC=${RISCV64_GCC:-$(command -v riscv64-linux-gnu-gcc || true)}
if [ -z "$RISCVC" ]; then
  RISCVC=$(command -v riscv64-unknown-elf-gcc || true)
fi
if [ -z "$RISCVC" ]; then
  echo "Error: riscv64 cross-compiler not found. Install riscv64-linux-gnu-gcc or riscv64-unknown-elf-gcc." >&2
  exit 1
fi

QEMU=${QEMU_RISCV:-$(command -v qemu-riscv64 || true)}
if [ -z "$QEMU" ]; then
  echo "Error: qemu-riscv64 not found. Install QEMU with RISC-V support." >&2
  exit 1
fi

if [ $# -lt 1 ]; then
    echo "Usage: $0 [options] <source.c> [input-string]" >&2
    echo "Options: -O1, -O2, --metrics" >&2
    exit 1
fi

# --- 1. Separate Flags from Arguments ---
COMPILER_FLAGS=""
while [[ $# -gt 0 && "$1" == -* ]]; do
    COMPILER_FLAGS="$COMPILER_FLAGS $1"
    shift
done

SRC_FILE="${1:-}"
INPUT="${2:-}"

if [ -z "$SRC_FILE" ] || [ ! -f "$SRC_FILE" ]; then
    echo "Error: source file '$SRC_FILE' not found." >&2
    exit 1
fi

# --- 2. Run Parser with Optimization & Metrics ---
# We pass the collected flags ($COMPILER_FLAGS) here
echo "--> Running Parser with flags: $COMPILER_FLAGS"
"$PARSER" $COMPILER_FLAGS "$SRC_FILE" > /dev/null

# --- 3. Assemble and Link ---
TMP_EXE="$(mktemp /tmp/qemu_run_XXXXXX.elf)"
TIME_OUTPUT="$(mktemp /tmp/qemu_time_XXXXXX.txt)"

cleanup_all() { rm -f "$TMP_EXE" "$TIME_OUTPUT"; }
trap cleanup_all EXIT

$RISCVC -static -o "$TMP_EXE" output.s

# --- 4. Execute with QEMU ---
echo "--> Executing in QEMU..."
if [ -n "$INPUT" ]; then
    printf '%s' "$INPUT" | /usr/bin/time -f "%e %M" -o "$TIME_OUTPUT" "$QEMU" "$TMP_EXE"
else
    /usr/bin/time -f "%e %M" -o "$TIME_OUTPUT" "$QEMU" "$TMP_EXE"
fi

# --- 5. Append Metrics if requested ---
if [[ "$COMPILER_FLAGS" == *"--metrics"* ]]; then
    if [ -s "$TIME_OUTPUT" ]; then
        READ_TIME=$(cat "$TIME_OUTPUT")
        EXEC_TIME=$(echo "$READ_TIME" | cut -d' ' -f1)
        PEAK_MEM=$(echo "$READ_TIME" | cut -d' ' -f2)
        
        if [ -f "compiler_metrics.txt" ]; then
            # Remove existing end marker if present
            sed -i '/==========================/d' compiler_metrics.txt
            echo "Execution time:                         $EXEC_TIME s" >> compiler_metrics.txt
            echo "Peak memory usage:                      $PEAK_MEM KB" >> compiler_metrics.txt
            echo "==========================" >> compiler_metrics.txt
        fi
    fi
fi