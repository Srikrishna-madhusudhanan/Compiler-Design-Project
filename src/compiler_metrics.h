#ifndef COMPILER_METRICS_H
#define COMPILER_METRICS_H

#include <stdio.h>
#include "ir.h"

typedef struct CompilerMetrics {
    int pre_opt_ir_instructions;
    int post_opt_ir_instructions;
    int post_opt_basic_blocks;
    int dce_removed_instructions;
    int dce_removed_definitions;
    int total_spilled_variables;
    int assembly_nonblank_lines;
    double execution_time_ns;
    long peak_memory_kb;
} CompilerMetrics;

void compiler_metrics_init(CompilerMetrics *m);

int compiler_metrics_count_ir_instructions(const IRProgram *prog);

int compiler_metrics_count_basic_blocks(const IRProgram *prog);

/* ra_results: NULL-terminated array from reg_alloc_program() */
void compiler_metrics_set_spill_total(CompilerMetrics *m, void *ra_results);

void compiler_metrics_read_assembly_lines(CompilerMetrics *m, const char *asm_path);

void compiler_metrics_fprint(const CompilerMetrics *m, FILE *fp);

void compiler_metrics_print_and_save(const CompilerMetrics *m, const char *path);

#endif
