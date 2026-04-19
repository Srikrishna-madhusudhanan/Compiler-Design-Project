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
  echo "Error: riscv64 cross-compiler not found." >&2
  exit 1
fi

QEMU=${QEMU_RISCV:-$(command -v qemu-riscv64 || command -v qemu-riscv64-static || true)}
if [ -z "$QEMU" ]; then
  echo "Error: qemu-riscv64 not found." >&2
  exit 1
fi

if [ $# -lt 1 ]; then
    echo "Usage: $0 [options] <source.c> [input-string]" >&2
    echo "Options: -O1, -O2, --metrics, --cleanup" >&2
    exit 1
fi

# --- GNU TIME CHECK ---
if ! /usr/bin/time -f "%e" echo test >/dev/null 2>&1; then
    echo "Error: GNU time not installed. Install package 'time'." >&2
    exit 1
fi

# --- Separate Flags ---
# FIX 1: Split --metrics out from compiler flags so it is never forwarded
#         to the compiler/parser. Previously ALL flags including --metrics
#         were concatenated into COMPILER_FLAGS and passed to $PARSER and
#         $RISCVC, causing "unrecognised option" errors.
COMPILER_FLAGS=()
SHOW_METRICS=false
CLEANUP=false

while [[ $# -gt 0 && "$1" == -* ]]; do
    if [[ "$1" == "--metrics" ]]; then
        SHOW_METRICS=true
    elif [[ "$1" == "--cleanup" ]]; then
        CLEANUP=true
    else
        COMPILER_FLAGS+=("$1")
    fi
    shift
done

SRC_FILE="${1:-}"
INPUT="${2:-}"

if [ -z "$SRC_FILE" ] || [ ! -f "$SRC_FILE" ]; then
    echo "Error: source file '$SRC_FILE' not found." >&2
    exit 1
fi

# --- Run Parser ---
echo "--> Running Parser with flags: ${COMPILER_FLAGS[*]:-<none>}"
# FIX 2: Use array expansion "${COMPILER_FLAGS[@]}" so flags with spaces
#        are passed as separate arguments, not a single glob-split string.
"$PARSER" "${COMPILER_FLAGS[@]}" "$SRC_FILE" > /dev/null

# FIX 3: Verify output.s was actually produced by the parser before linking.
#        Without this check, a stale output.s from a prior run gets linked
#        silently, producing a binary that does not match the current source.
if [ ! -s output.s ]; then
    echo "Error: parser produced no output (output.s missing or empty)." >&2
    exit 1
fi

# --- Assemble & Link ---
TMP_EXE="$(mktemp /tmp/qemu_run_XXXXXX.elf)"
TIME_OUTPUT="$(mktemp /tmp/qemu_time_XXXXXX.txt)"

# FIX 4: Also trap SIGINT and SIGTERM so temporaries are removed when the
#        user hits Ctrl-C or the process is killed, not only on normal exit.
cleanup_all() { rm -f "$TMP_EXE" "$TIME_OUTPUT"; }
trap cleanup_all EXIT INT TERM

# FIX 5: Pass only the real compiler flags (not --metrics) to the compiler.
$RISCVC "${COMPILER_FLAGS[@]}" -static -o "$TMP_EXE" output.s src/exception_runtime.s

# --- EXECUTION WITH REAL BENCHMARK METRICS ---
echo "--> Executing in QEMU..."

QEMU_EXIT=0
if [ -n "$INPUT" ]; then
    printf '%s' "$INPUT" | /usr/bin/time -v -o "$TIME_OUTPUT" "$QEMU" "$TMP_EXE" || QEMU_EXIT=$?
else
    /usr/bin/time -v -o "$TIME_OUTPUT" "$QEMU" "$TMP_EXE" || QEMU_EXIT=$?
fi

# --- METRICS EXTRACTION ---
if [ "$SHOW_METRICS" = true ]; then
    if [ -s "$TIME_OUTPUT" ]; then

        # Convert elapsed time (h:mm:ss.ss or m:ss.ss or ss.ss) to milliseconds
        EXEC_TIME_MS=$(grep "Elapsed (wall clock) time" "$TIME_OUTPUT" | awk -F': ' '{print $2}' | awk -F: '{
            if(NF==3) print ($1*3600+$2*60+$3)*1000;
            else if(NF==2) print ($1*60+$2)*1000;
            else print $1*1000;
        }')
        # Extract peak RSS in kbytes
        PEAK_MEM=$(grep "Maximum resident set size" "$TIME_OUTPUT" | awk '{print $NF}')

        echo ""
        echo "===== EXECUTION METRICS ====="
        echo "Elapsed time    : ${EXEC_TIME_MS:-0} ms"
        echo "Peak memory     : ${PEAK_MEM:-0} KB"
        echo "============================="
        echo ""

        # Write metrics into compiler_metrics.txt (create if missing)
        if [ ! -f "compiler_metrics.txt" ]; then
            touch compiler_metrics.txt
        fi

        # Strip old exec/mem lines, then append fresh ones
        TMP_METRICS="$(mktemp /tmp/compiler_metrics_XXXXXX.txt)"
        grep -v -e "^Execution time" -e "^Peak memory" compiler_metrics.txt > "$TMP_METRICS" || true
        mv "$TMP_METRICS" compiler_metrics.txt

        cat >> compiler_metrics.txt <<EOF
Execution time (wall):               ${EXEC_TIME_MS:-0} ms
Peak memory usage:                   ${PEAK_MEM:-0} KB
==========================
EOF
    else
        echo "---> Warning: Metrics output file is empty (QEMU may have failed with code $QEMU_EXIT)."
    fi
fi

if [ "$CLEANUP" = true ]; then
    echo "--> Cleaning up generated files..."
    rm -f *.json *.dot
fi