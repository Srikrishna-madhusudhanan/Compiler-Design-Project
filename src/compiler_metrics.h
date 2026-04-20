#ifndef COMPILER_METRICS_H
#define COMPILER_METRICS_H

#include <stdio.h>
#include "ir.h"

/* Per-function spill statistics */
typedef struct {
    char *func_name;
    int spilled_variables;
    int spill_loads;
    int spill_stores;
} FunctionSpillStats;

typedef struct CompilerMetrics {
    /* IR Code Size */
    int pre_opt_ir_instructions;
    int post_opt_ir_instructions;
    int post_opt_basic_blocks;
    int cfg_edges;
    int loops_found;
    
    /* Optimization */
    int dce_removed_instructions;
    int dce_removed_definitions;
    int const_folded;
    int copy_propagated;
    double ir_growth_factor;
    
    /* Register Allocation */
    int total_spilled_variables;
    int total_spill_loads;
    int total_spill_stores;
    int max_register_pressure;
    int interference_graph_nodes;
    
    /* Per-function spill tracking */
    FunctionSpillStats *func_spill_stats;
    int func_spill_count;
    
    /* Assembly */
    int assembly_nonblank_lines;
    int assembly_total_instructions;
    int assembly_loads;
    int assembly_stores;
    int assembly_alu_ops;
    int assembly_branches;
    double memory_intensity;
    
    /* Timing & Memory */
    double compilation_time_ms;
    double avg_execution_time_ms;
    long dynamic_instruction_count;
    long peak_memory_kb;
    
    /* Output binary path for dynamic execution */
    char *output_binary_path;
} CompilerMetrics;

void compiler_metrics_init(CompilerMetrics *m);

int compiler_metrics_count_ir_instructions(const IRProgram *prog);

void compiler_metrics_update_cfg_stats(CompilerMetrics *m, const IRProgram *prog);

/* ra_results: NULL-terminated array from reg_alloc_program() */
void compiler_metrics_set_spill_total(CompilerMetrics *m, void *ra_results);

void compiler_metrics_read_assembly_lines(CompilerMetrics *m, const char *asm_path);

void compiler_metrics_fprint(const CompilerMetrics *m, FILE *fp);

void compiler_metrics_print_and_save(const CompilerMetrics *m, const char *path);

/* Analyze assembly file for instruction type breakdown */
void compiler_metrics_analyze_assembly(CompilerMetrics *m, const char *asm_path);

#endif
