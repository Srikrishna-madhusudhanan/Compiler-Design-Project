# Final Project Change Report: PaniniC Compiler Upgrade

This report provides a line-by-line summary of every modification and new file addition made during this session. The project has been enhanced with performance metrics, animated visualizations (AST, CFG, RA), and a premium, resizable web GUI branded as **PaniniC**.

---

## 1. Modified Infrastructure Files

### [src/compiler_metrics.h](file:///home/bms/apr12/Compiler-Design-Project/src/compiler_metrics.h)
- **Modifications (Lines 13-16)**: Added `double execution_time_s` and `long peak_memory_kb` to the `CompilerMetrics` structure.

### [src/compiler_metrics.c](file:///home/bms/apr12/Compiler-Design-Project/src/compiler_metrics.c)
- **Modifications (Lines 76-80)**: Updated `compiler_metrics_fprint` to display new performance metrics.

### [src/ir.h](file:///home/bms/apr12/Compiler-Design-Project/src/ir.h)
- **Addition (Line 160)**: Declared `ir_snprint_instr` to support string conversion of instructions for JSON export.

### [src/ir.c](file:///home/bms/apr12/Compiler-Design-Project/src/ir.c)
- **Addition (Lines 400-475)**: Implemented `ir_snprint_instr` using `snprintf` to enable structured export of basic block contents.

### [scripts/qemu_run.sh](file:///home/bms/apr12/Compiler-Design-Project/scripts/qemu_run.sh)
- **Major Rewrite (Lines 54-82)**: Integrated `/usr/bin/time` to capture real-time execution metrics.

---

## 2. Advanced Visualization Logic

### [src/ir_opt.h](file:///home/bms/apr12/Compiler-Design-Project/src/ir_opt.h)
- **Addition (Line 60)**: Declared `export_cfg_to_json` for Control Flow Graph visualization.

### [src/ir_opt.c](file:///home/bms/apr12/Compiler-Design-Project/src/ir_opt.c)
- **Addition (Lines 125-160)**: Implemented `export_cfg_to_json` to serialize CFG blocks and edges.
- **Modification (Lines 1265-1275)**: Integrated CFG export into the main `optimize_program` pipeline.

### [src/ast.c](file:///home/bms/apr12/Compiler-Design-Project/src/ast.c)
- **Addition (Lines 143-189)**: Implemented JSON serialization for AST nodes.

### [src/reg_alloc.c](file:///home/bms/apr12/Compiler-Design-Project/src/reg_alloc.c)
- **Addition (Lines 738-775)**: Implemented JSON export for the interference graph and simplification stack.

---

## 3. PaniniC Web GUI (Refined)

### [gui/server.js](file:///home/bms/apr12/Compiler-Design-Project/gui/server.js)
- **Modifications**: Added collection and cleanup logic for `_cfg.json` files.

### [gui/public/index.html](file:///home/bms/apr12/Compiler-Design-Project/gui/public/index.html)
- **Additions**: 
    - Dedicated **Control Flow Graph** panel.
    - **Panel Control Buttons**: Minimize/Maximize triggers for every panel.

### [gui/public/styles.css](file:///home/bms/apr12/Compiler-Design-Project/gui/public/styles.css)
- **Major Additions (Lines 158-250)**:
    - `.minimized` and `.maximized` layout logic.
    - Glassmorphism buttons for panel controls.
    - Specific styling for CFG nodes to match the user's requested visual style.

### [gui/public/app.js](file:///home/bms/apr12/Compiler-Design-Project/gui/public/app.js)
- **Key Enhancements**:
    - `renderCFG()`: New vis-network implementation for block-based flow visualization.
    - **RA Stability Fix**: Automatically disables physics in Register Allocation graphs after stabilization to prevent "squiggling".
    - **UI State Management**: Handlers for all minimize/maximize actions.

---

## Final Configuration
- **Project Name**: PaniniC
- **Visualizations**: AST (Tree), CFG (Flow), RA (Interference Graph).
- **Interactive UI**: Fully resizable with minimize/maximize capabilities for a premium demo experience.
