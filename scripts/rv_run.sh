#!/bin/bash
set -euo pipefail

ROOT_DIR=$(cd "$(dirname "$0")/.." && pwd)
cd "$ROOT_DIR"

# Tools
PARSER=./build/parser
RVAS=./tools/rvas/rvas
RVLD=./tools/rvld/rvld
QEMU=${QEMU_RISCV:-$(command -v qemu-riscv64 || command -v qemu-riscv64-static || true)}

# Ensure tools are built
make -C tools/rvas > /dev/null
make -C tools/rvld > /dev/null
if [ ! -x "$PARSER" ]; then
    make parser > /dev/null
fi

if [ -z "$QEMU" ]; then
    echo "Error: qemu-riscv64 not found." >&2
    exit 1
fi

if [ $# -lt 1 ]; then
    echo "Usage: $0 [options] <source.c> [input-string]" >&2
    exit 1
fi

COMPILER_FLAGS=()
while [[ $# -gt 0 && "$1" == -* ]]; do
    COMPILER_FLAGS+=("$1")
    shift
done

SRC_FILE="${1:-}"
INPUT="${2:-}"

if [ -z "$SRC_FILE" ] || [ ! -f "$SRC_FILE" ]; then
    echo "Error: source file '$SRC_FILE' not found." >&2
    exit 1
fi

# 1. Compile C to Assembly
echo "--> Compiling $SRC_FILE..."
"$PARSER" "${COMPILER_FLAGS[@]}" "$SRC_FILE" > /dev/null

if [ ! -s output.s ]; then
    echo "Error: parser produced no output." >&2
    exit 1
fi

# 2. Assemble everything
echo "--> Assembling..."
mkdir -p build/obj
"$RVAS" src/io_runtime.s -o build/obj/io_runtime.o
"$RVAS" src/exception_runtime.s -o build/obj/exception_runtime.o
"$RVAS" output.s -o build/obj/output.o

# 3. Link
echo "--> Linking..."
"$RVLD" -o build/a.out build/obj/io_runtime.o build/obj/exception_runtime.o build/obj/output.o

# 4. Run
echo "--> Executing in QEMU..."
if [ -n "$INPUT" ]; then
    printf '%s' "$INPUT" | "$QEMU" build/a.out
else
    "$QEMU" build/a.out
fi
