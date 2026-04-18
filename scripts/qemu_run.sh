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
$RISCVC "${COMPILER_FLAGS[@]}" -static -o "$TMP_EXE" output.s

# --- EXECUTION WITH REAL BENCHMARK METRICS ---
echo "--> Executing in QEMU..."

# FIX 6: Use $'...' quoting so \n is an actual newline character.
#        With a plain double-quoted string, GNU time receives the literal
#        two-character sequence \n and writes everything on one line,
#        causing every subsequent "grep ^real / ^user / ^sys / ^mem" to
#        find nothing and leave the variables empty.
TIME_FORMAT=$'real %e\nuser %U\nsys %S\nmem %M'

if [ -n "$INPUT" ]; then
    printf '%s' "$INPUT" | /usr/bin/time -f "$TIME_FORMAT" -o "$TIME_OUTPUT" "$QEMU" "$TMP_EXE"
else
    /usr/bin/time -f "$TIME_FORMAT" -o "$TIME_OUTPUT" "$QEMU" "$TMP_EXE"
fi

# --- METRICS EXTRACTION ---
if [ "$SHOW_METRICS" = true ]; then
    if [ -s "$TIME_OUTPUT" ]; then

        EXEC_TIME=$(grep "^real" "$TIME_OUTPUT" | awk '{print $2}')
        USER_TIME=$(grep "^user" "$TIME_OUTPUT" | awk '{print $2}')
        SYS_TIME=$(grep  "^sys"  "$TIME_OUTPUT" | awk '{print $2}')
        PEAK_MEM=$(grep  "^mem"  "$TIME_OUTPUT" | awk '{print $2}')

        echo ""
        echo "===== EXECUTION METRICS ====="
        echo "Wall clock time : $EXEC_TIME sec"
        echo "User CPU time   : $USER_TIME sec"
        echo "Sys CPU time    : $SYS_TIME sec"
        echo "Peak memory     : $PEAK_MEM KB"
        echo "============================="
        echo ""

        if [ -f "compiler_metrics.txt" ]; then
            # FIX 7: Safer in-place edit using a single atomic temp-file swap.
            #        The old two-step grep-v pipeline wrote to compiler_metrics.tmp
            #        then back to compiler_metrics.txt in two separate commands;
            #        a crash between them left a truncated or missing metrics file.
            #        Using a single sed -i (or awk to a tmp then mv) is atomic.
            TMP_METRICS="$(mktemp /tmp/compiler_metrics_XXXXXX.txt)"
            grep -v -e "^Execution time" -e "^Peak memory" compiler_metrics.txt \
                > "$TMP_METRICS" || true
            mv "$TMP_METRICS" compiler_metrics.txt

            # Multiply by 1,000,000,000 for nanoseconds. 
            # We use bc for floating point math if available, or just append zeros if it's a simple float.
            # Simpler: use awk to do the multiplication.
            EXEC_TIME_NS=$(echo "$EXEC_TIME" | awk '{print int($1 * 1000000000)}')

            cat >> compiler_metrics.txt <<EOF
Execution time (wall):               $EXEC_TIME_NS ns
Execution time (user):               $USER_TIME s
Execution time (sys):                $SYS_TIME s
Peak memory usage:                   $PEAK_MEM KB
==========================
EOF
        fi
    else
        echo "--> Warning: Metrics output file is empty."
    fi
fi

if [ "$CLEANUP" = true ]; then
    echo "--> Cleaning up generated files..."
    rm -f *.json *.dot
fi