# 🚀 PaniniC — C-Subset to RISC-V Compiler

A full compiler pipeline from a C-subset language to RISC-V assembly, with a premium interactive visualization dashboard. Developed as part of the Compiler Design Lab course.

## Pipeline Overview

```
Source Code → Lexer → Parser → Semantic Analysis → IR → IR Optimization
           → Instruction Scheduling → Register Allocation → RISC-V Codegen-> Assembler -> Linker -> ELF file (final executable)
```

---

## Project Directory Structure

```text
.
├── Makefile              # Build rules for compiler and custom toolchain
├── src/                  # Compiler source code (Lexer, Parser, AST, IR, Semantic, RISC-V Codegen)
│   ├── minilib.c         # Custom minimal C runtime
│   ├── exception_runtime.s # Exception handling assembly runtime
│   └── io_runtime.s      # Base I/O assembly runtime
├── tools/                # Custom RISC-V Toolchain
│   ├── rvas/             # Custom RISC-V Assembler
│   └── rvld/             # Custom RISC-V Static Linker
├── test/                 # C-subset test suite
│   ├── basics/           # Basic arithmetic and control flow
│   ├── complex/          # Advanced algorithms (fibonacci, bst, factorial)
│   ├── features/         # Language specific features (switch, loops)
│   ├── oop/              # Object-oriented programming features
│   └── optimizations/    # Tests specifically for loop unrolling, etc.
├── scripts/              # Helper shell scripts
│   ├── run_tests.sh      # Compiles all tests and checks for success
│   ├── run_qemu_tests.sh # Compiles and runs test suite in QEMU simulator
│   ├── rv_run.sh         # Executes the complete custom toolchain pipeline
│   └── cleanup.sh        # Cleans up generated visualization artifacts
├── gui/                  # Interactive visualization web dashboard
└── docs/                 # Project documentation and reports
```

---

## Prerequisites

Install the standard build tools, QEMU for RISC-V simulation, and the RISC-V GCC cross-compiler (used to compile the minimal C runtime library).

```bash
# On Ubuntu/Debian
sudo apt update
sudo apt install build-essential qemu-user qemu-user-static gcc-riscv64-linux-gnu time
```
*(Node.js and npm are also required if you plan to use the interactive Web GUI.)*

```bash
# On Ubuntu/Debian
sudo apt update
sudo apt install nodejs npm
```

---

## Quick Start

### 1. Build the Compiler
```bash
make clean && make all
```

### 2. Compile a Program and View Output In Terminal
```bash
./build/parser < test/oop/polymorphism_demo.c
# Optional flags:
# --metrics   → save timing/memory to compiler_metrics.txt
# -O0/-O1/-O2 → optimization level
```
You can view the pre-optimized IR code in `ir.txt` and optimized IR code in `ir_opt.txt`.

You can view the assembly (RISC-V) code in the `output.s` file.

### 3. Compile & Run a Single File with QEMU 
The command below will:
1. Compile a test program from source code to RISC-V
2. Assemble it into object file (.o)
3. Link it, and run the ELF executable in QEMU (a RISC-V Simulator).
```bash
./scripts/rv_run.sh test/complex/factorial_tail_recursive.c
# Optional flags:
#   --metrics   → save timing/memory to compiler_metrics.txt
#   -O0/-O1/-O2 → optimization level
```

### 4. To Run All Tests
```bash
./scripts/run_tests.sh #checks if compilation succeeds
# or with QEMU:
./scripts/run_qemu_tests.sh #checks if compilation and execution succeeds
```

### 5. To Cleanup The Generated .dot and .json files (which were created for GUI visualization):
```bash
./scripts/cleanup.sh
```


---

## 🎨 Interactive Visualization Dashboard

PaniniC ships with a modern web GUI that animates every stage of compilation.

### Setup

```bash
# Install dependencies (one-time)
cd gui && npm install && cd ..

# Build the compiler
make all

# Start the dashboard
cd gui && npm start
```

Open **[http://localhost:3000](http://localhost:3000)** in your browser.

---

### Using the Dashboard

#### Step 1 — Write or Load Code
- Type directly in the **Source Code** editor on the left, or
- Select a built-in example from the **Load Example…** dropdown in the header.

#### Step 2 — Choose Optimization Level
- Use the **O0 / O1 / O2** dropdown in the header.
- Use **O0** to see more variables survive into Register Allocation.

#### Step 3 — Compile
- Click **Compile & Viz**.
- The compiler runs, the **Terminal Output** panel shows live progress, and the following are populated automatically:
  - **Intermediate Representation** panel (left)
  - **Control Flow Graph** (center)
  - **Performance Metrics** dashboard
  - **Register Usage Map** (right panel, bottom)

---

### Visualizations

#### 📐 AST Visualization
- Click **Animate Build** in the AST panel (top center).
- Nodes appear one by one, building the Abstract Syntax Tree.

#### 🌐 Control Flow Graph
- Auto-renders after compilation showing basic blocks and control edges.

#### ⚡ Register Allocation (Chaitin-Briggs)
Located in the **right pane**, top panel.

1. Click **Compile & Viz** first.
2. Click **Animate Trace**.
3. Watch the animation:
   - **Simplify phase**: nodes are pushed onto the stack (dimmed in the graph)
   - **Select phase**: nodes are popped and colored by assigned physical register
   - **Spilled nodes** appear in red
4. The interference graph shows all variables as glowing neon nodes.
5. Hover over a node to see its register assignment.

> **Tip**: Click the **▢** (maximize) button on the panel header to expand it full-screen.

#### 🗓 Instruction Scheduling (Dependency DAG)
Located in the **right pane**, middle panel (scroll down).

1. Click **Compile & Viz** first.
2. Click **Animate Sched**.
3. The dependency DAG appears (Top-Down hierarchy), then nodes light up in scheduling order:

| Color | Meaning |
|-------|---------|
| 🟠 Orange | Critical path — must schedule first |
| 🟣 Purple | High priority |
| 🔵 Blue | Mid priority |
| 🟢 Green | Low priority |

4. Dependency arrows glow in the matching color as each instruction is selected.
5. The **Terminal** logs each scheduled instruction with its priority tier.

#### 📊 Performance Metrics
- Auto-populated after compilation: pre/post-opt IR counts, exec time (ns), peak memory (KB).
- Bar chart compares pre- vs post-optimization instruction counts.

---

## 🔧 RISC-V Toolchain (rvas + rvld)

PaniniC includes a custom RISC-V assembler (`rvas`) and static linker (`rvld`). The toolchain connects the compiled assembly with `minilib.c`, a custom minimal runtime environment.

The steps below are performed by `scripts/rv_run.sh` altogether, but we have listed it below for testing the assembler and linker process step by step.

Before executing the steps below, ensure you have the compiled output (in `output.s` file).
```bash
# Ensure the tools are built
make -C tools/rvas
make -C tools/rvld

# Assemble the runtime and the program (assuming program is compiled to output.s)
mkdir -p build/obj
./tools/rvas/rvas src/io_runtime.s -o build/obj/io_runtime.o
./tools/rvas/rvas src/exception_runtime.s -o build/obj/exception_runtime.o
./tools/rvas/rvas output.s -o build/obj/output.o

# Compile the minimal C runtime
riscv64-linux-gnu-gcc -c src/minilib.c -o build/obj/minilib.o -ffreestanding -fno-builtin -nostdlib -O2

# Link to create the final executable
./tools/rvld/rvld -o build/program.elf \
    build/obj/io_runtime.o \
    build/obj/minilib.o \
    build/obj/exception_runtime.o \
    build/obj/output.o

# Run via QEMU
qemu-riscv64 build/program.elf
```

---

## ⚠️ Exception Handling

PaniniC supports `try`, `catch`, and `throw` statements compiled down to RISC-V via `setjmp`/`longjmp`.

```c
try {
    throw 42;
} catch (int e) {
    // handle e
}
```

See `test/exception_test.c` for examples.

---

## Meet the Team

| Name | Roll No |
|------|---------|
| Adithya Ananth | CS23B001 |
| Srikrishna Madhusudhanan | CS23B056 |
| Sudhanva Bharadwaj B M | CS23B051 |
