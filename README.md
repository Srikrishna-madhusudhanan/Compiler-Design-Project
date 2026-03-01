# Compiler From C-Subset to RISC-V

This project implements a C-subset compiler developed as part of our Compiler Design Lab course.

Our project is to develop a compiler that translates programs written in a custom language (whose allowed instructions are a subset of the C programming language) into executable RISC-V assembly code. We will also apply selected optimizations to improve the efficiency of the generated code.

The implementation follows the classical compiler pipeline:

- Lexical Analysis
- Syntax Analysis (Parsing)
- Semantic Analysis
- **Intermediate Representation (IR)** — Three-address code (Week 4)
- Code Generation (RISC-V)
- Basic Optimizations

### Week 4: IR (Three-Address Code)

The compiler generates a simple three-address IR from the AST. After semantic analysis, run:

```bash
./parser <source.c>
```

IR is printed to stdout and exported to `ir.txt`. Run validation:

```bash
./run_tests.sh
```

## Meet the Team
- Adithya Ananth (CS23B001)
- Srikrishna Madhusudhanan (CS23B056)
- Sudhanva Bharadwaj B M (CS23B051)
