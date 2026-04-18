# 🚀 PaniniC — C-Subset to RISC-V Compiler

A full compiler pipeline from a C-subset language to RISC-V assembly, with a premium interactive visualization dashboard. Developed as part of the Compiler Design Lab course.

## Pipeline Overview

```
Source Code → Lexer → Parser → Semantic Analysis → IR → Optimization
           → Instruction Scheduling → Register Allocation → RISC-V Codegen
```

---

## Quick Start

### 1. Build the Compiler
```bash
make clean && make
```

### 2. Compile a Program
```bash
./build/parser < test/basics/minimal_main.c
```

### 3. Run All Tests
```bash
./run_tests.sh
# or with QEMU:
./scripts/run_qemu_tests.sh
```

### 4. Compile & Run a Single File with QEMU
```bash
./scripts/qemu_run.sh test/complex/factorial_tail_recursive.c "7\n"
# Optional flags:
#   --metrics   → save timing/memory to compiler_metrics.txt
#   -O0/-O1/-O2 → optimization level
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
  - **Performance Metrics** dashboard with pre/post-opt IR counts
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

PaniniC includes a custom assembler and static linker.

```bash
# Build both tools
make toolchain

# Assemble
./tools/rvas/rvas -o program.o program.s

# Link
./tools/rvld/rvld -o program program.o tools/rvld/tests/crt0.o

# Run via QEMU
qemu-riscv64 ./program

# Run linker test suite
cd tools/rvld/tests && ./run_tests.sh
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
