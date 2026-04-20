#!/bin/bash
set -euo pipefail

ROOT_DIR=$(cd "$(dirname "$0")/.." && pwd)
cd "$ROOT_DIR"

# Tools
PARSER=./build/parser
RVAS=./tools/rvas/rvas
RVLD=./tools/rvld/rvld
QEMU=${QEMU_RISCV:-$(command -v qemu-riscv64 || command -v qemu-riscv64-static || true)}

# Library Paths - Hardcoded for this environment based on research
LIBC_DIR="/usr/riscv64-linux-gnu/lib"
LIBGCC_DIR="/usr/lib/gcc-cross/riscv64-linux-gnu/13"

# Ensure tools are built
echo "--> Building tools..."
make -C tools/rvas > /dev/null
make -C tools/rvld > /dev/null
make parser > /dev/null

if [ -z "$QEMU" ]; then
    echo "Error: qemu-riscv64 not found." >&2
    exit 1
fi

if [ $# -lt 1 ]; then
    echo "Usage: $0 [options] <source.c> [input-string]" >&2
    exit 1
fi

COMPILER_FLAGS=()
WANT_METRICS=0
while [[ $# -gt 0 && "$1" == -* ]]; do
    COMPILER_FLAGS+=("$1")
    if [ "$1" = "--metrics" ]; then
        WANT_METRICS=1
    fi
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
# Compile our custom minilib
riscv64-linux-gnu-gcc -c src/minilib.c -o build/obj/minilib.o -ffreestanding -fno-builtin -nostdlib -O2
"$RVAS" output.s -o build/obj/output.o

# 3. Link with our custom minilib
echo "--> Linking with custom minilib..."
"$RVLD" -o build/program.elf \
    build/obj/io_runtime.o \
    build/obj/minilib.o \
    build/obj/exception_runtime.o \
    build/obj/output.o


# 4. Run
echo -e "--> Executing in QEMU...\n"

# If metrics were requested, also run with instruction tracing
if [ $WANT_METRICS -eq 1 ] && command -v qemu-riscv64 &> /dev/null; then
    # Run with instruction trace for dynamic instruction count
    if [ -n "$INPUT" ]; then
        printf '%s' "$INPUT" | /usr/bin/time -v "$QEMU" -d in_asm ./build/program.elf 2> /tmp/qemu_trace.txt || true
    else
        /usr/bin/time -v "$QEMU" -d in_asm ./build/program.elf 2> /tmp/qemu_trace.txt || true
    fi
    
    # Extract peak memory from time output (which goes to stderr along with qemu trace)
    if [ -f /tmp/qemu_trace.txt ]; then
        PEAK_MEM=$(grep "Maximum resident set size" /tmp/qemu_trace.txt | awk '{print $6}')
        if [ -z "$PEAK_MEM" ]; then PEAK_MEM=0; fi
        
        # Count dynamic instructions from trace
        DYNAMIC_INSTR=$(grep -c "^0x" /tmp/qemu_trace.txt || echo "0")
        echo -e "\nDynamic instruction count: $DYNAMIC_INSTR"
        
        if [ -f compiler_metrics.txt ]; then
            sed -i "s/Dynamic instructions: *0/Dynamic instructions:          $DYNAMIC_INSTR/" compiler_metrics.txt
            sed -i "s/Peak memory: *0 KB/Peak memory:                   $PEAK_MEM KB/" compiler_metrics.txt
        fi
    fi
else
    # Normal execution without tracing
    if [ -n "$INPUT" ]; then
        printf '%s' "$INPUT" | "$QEMU" build/program.elf
    else
        "$QEMU" build/program.elf
    fi
fi

# Display metrics if generated
if [ $WANT_METRICS -eq 1 ] && [ -f compiler_metrics.txt ]; then
    echo -e "\n"
    cat compiler_metrics.txt
fi
