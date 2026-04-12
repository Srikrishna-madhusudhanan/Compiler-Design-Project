# Compiler From C-Subset to RISC-V

This project implements a C-subset compiler developed as part of our Compiler Design Lab course.

Our project is to develop a compiler that translates programs written in a custom language (whose allowed instructions are a subset of the C programming language) into executable RISC-V assembly code. We will also apply selected optimizations to improve the efficiency of the generated code.

The implementation follows the classical compiler pipeline:

- Lexical Analysis
- Syntax Analysis (Parsing)
- Semantic Analysis
- Intermediate Representation (IR)
- Code Generation (RISC-V)
- Basic Optimizations


The compiler generates a simple three-address IR from the AST. After semantic analysis, run the following:

```bash
make clean
make
./build/parser < ./test/<source.c>
```

IR is printed to stdout and exported to `ir.txt`. Run validation:

```bash
./run_tests.sh
```

Run QEMU-backed regression tests:

```bash
./scripts/run_qemu_tests.sh
```

Compile and run a single C test with QEMU via the helper pipeline:

```bash
./scripts/qemu_run.sh test/complex/factorial_tail_recursive.c "7\n"
```

You can also add these flags to the above command:
- Include metrics information (which will be stored in `compiler_metrics.txt`) using `--metrics`.
- Choose the optimization level (0, 1 or 2): `-O0`, `-O1`, `-O2`.


## Interactive Visualization Dashboard (PaniniC)

PaniniC comes with a modern, interactive web-based GUI to visualize the compilation process, performance metrics, and register allocation.

### Features
- **Live Code Editor**: Write and compile code directly from the browser.
- **AST Visualization**: Animate the construction of the Abstract Syntax Tree.
- **Register Allocation Animation**: Visualize the graph-coloring and simplification phases.
- **Performance Dashboards**: Real-time charts for instruction counts, execution time, and memory usage.
- **Resizable Interface**: Customize your workspace with adjustable panes.

### How to Run
1. **Ensure dependencies are installed**:
   ```bash
   cd gui
   npm install
   cd ..
   ```
2. **Build the compiler**:
   ```bash
   make all
   ```
3. **Start the GUI server**:
   ```bash
   node gui/server.js
   ```
4. **Access the dashboard**:
   Open [http://localhost:3000](http://localhost:3000) in your web browser.

> [!TIP]
> To see detailed Register Allocation animations, use the **-O0 (No Optimization)** setting in the GUI, as aggressive optimizations may remove variables in simpler programs.


## Meet the Team
- Adithya Ananth (CS23B001)
- Srikrishna Madhusudhanan (CS23B056)
- Sudhanva Bharadwaj B M (CS23B051)
