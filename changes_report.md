# Final Project Change Report: PaniniC Compiler Upgrade

This report provides a line-by-line summary of every modification and new file addition made during this session. The project has been enhanced with performance metrics, animated visualizations, and a premium, resizable web GUI branded as **PaniniC**.

---

## 1. Modified Infrastructure Files

### [src/compiler_metrics.h](file:///home/bms/apr12/Compiler-Design-Project/src/compiler_metrics.h)
- **Modifications (Lines 13-16)**: Added `double execution_time_s` and `long peak_memory_kb` to the `CompilerMetrics` structure to track program execution data.

### [src/compiler_metrics.c](file:///home/bms/apr12/Compiler-Design-Project/src/compiler_metrics.c)
- **Modifications (Lines 76-80)**: Updated `compiler_metrics_fprint` to display the new execution time and peak memory usage in the metrics report.

### [scripts/qemu_run.sh](file:///home/bms/apr12/Compiler-Design-Project/scripts/qemu_run.sh)
- **Major Rewrite (Lines 54-82)**: 
    - Replaced the basic QEMU execution logic with a `/usr/bin/time` wrapper.
    - Added logic to parse execution time and peak resident set size.
    - Added a routine to append these results to `compiler_metrics.txt` when the `--metrics` flag is present.

---

## 2. Modified Compiler Logic (JSON Animations)

### [src/ast.h](file:///home/bms/apr12/Compiler-Design-Project/src/ast.h)
- **Addition (Line 102)**: Declared `export_ast_to_json` to enable structured AST exports for the web frontend.

### [src/ast.c](file:///home/bms/apr12/Compiler-Design-Project/src/ast.c)
- **Addition (Lines 143-189)**:
    - `static void generate_json(...)`: Implemented a recursive JSON serialization helper.
    - `void export_ast_to_json(...)`: Implemented the public API for exporting the AST as JSON.

### [src/reg_alloc.h](file:///home/bms/apr12/Compiler-Design-Project/src/reg_alloc.h)
- **Addition (Line 124)**: Declared `reg_alloc_export_json` to support animated register allocation visualization.

### [src/reg_alloc.c](file:///home/bms/apr12/Compiler-Design-Project/src/reg_alloc.c)
- **Modifications (Lines 598-612)**: Updated `allocate_function` to catch the final coloring stack from the `simplify` phase and trigger the JSON export.
- **Addition (Lines 738-775)**: Implemented `reg_alloc_export_json`, which serializes the interference graph (nodes/edges) and the simplification stack.

### [src/parser.y](file:///home/bms/apr12/Compiler-Design-Project/src/parser.y)
- **Modification (Line 905)**: Added the final call to `export_ast_to_json` in the compiler's `main` function.

---

## 3. New Files (PaniniC Web GUI)

### [gui/server.js](file:///home/bms/apr12/Compiler-Design-Project/gui/server.js)
**Purpose**: The backend API for the PaniniC dashboard.
- **Features**:
    - Serves the static PaniniC frontend.
    - `/api/compile`: Orchestrates the parser, cleans up stale JSON data, and returns IR/AST/RA results.
    - `/api/run`: Executes the compiled code via the updated `qemu_run.sh` and returns performance data.
    - Identifies itself as the "PaniniC GUI server".

### [gui/public/index.html](file:///home/bms/apr12/Compiler-Design-Project/gui/public/index.html)
**Purpose**: The user interface.
- **Features**:
    - Premium "PaniniC Compiler" branding.
    - Nested flexbox layout supporting **Resizable Panes**.
    - Performance Dashboard with card-based metrics and a Chart.js canvas.
    - Dual visualization containers for AST and RA.

### [gui/public/styles.css](file:///home/bms/apr12/Compiler-Design-Project/gui/public/styles.css)
**Purpose**: The visual design system.
- **Features**:
    - Slate-themed dark mode using CSS variables.
    - **Splitter styles**: `.resizer-h` and `.resizer-v` with hover/dragging highlights.
    - Metrics card styling with semi-transparent backgrounds and consistent accents.

### [gui/public/app.js](file:///home/bms/apr12/Compiler-Design-Project/gui/public/app.js)
**Purpose**: The client-side logic.
- **Features**:
    - **Custom Resizer Engine**: Handles the dragging and resizing logic for all UI panels.
    - **AST Tree Growth Animation**: Sequentially builds the AST visually using `vis-network`.
    - **RA Coloring Animation**: Replays the coloring process by popping the stack and updating node colors in real-time.
    - **Metrics Handling**: Parses raw compiler text output into graphical data for Chart.js.

---

## Final Configuration
- **Project Name**: PaniniC
- **New Directory**: `gui/` (Self-contained web application)
- **New Tools**: Node.js, Express, Chart.js, vis-network.
- **Enhanced Capability**: 5 new JSON-based visualization endpoints and 2 new performance KPIs.
