#include "compiler_metrics.h"
#include "ir_opt.h"
#include "reg_alloc.h"
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>

void compiler_metrics_init(CompilerMetrics *m) {
    if (m) memset(m, 0, sizeof(*m));
}

int compiler_metrics_count_ir_instructions(const IRProgram *prog) {
    if (!prog) return 0;
    int n = 0;
    IRInstr *g = prog->global_instrs;
    while (g) {
        n++;
        g = g->next;
    }
    for (IRFunc *f = prog->funcs; f; f = f->next) {
        for (IRInstr *in = f->instrs; in; in = in->next)
            n++;
    }
    return n;
}

void compiler_metrics_update_cfg_stats(CompilerMetrics *m, const IRProgram *prog) {
    if (!m || !prog) return;
    int blocks = 0;
    int edges = 0;
    int instrs = 0;
    for (IRFunc *f = prog->funcs; f; f = f->next) {
        CFG *cfg = build_cfg(f);
        if (!cfg) continue;
        for (BasicBlock *bb = cfg->blocks; bb; bb = bb->next) {
            blocks++;
            edges += bb->succ_count;
            // printf("Block %d succ_count: %d\n", bb->id, bb->succ_count);
            for (IRInstr *in = bb->instrs; in; in = in->next) {
                instrs++;
                if (in == bb->last) break;
            }
        }
        free_cfg(cfg);
    }
    m->post_opt_basic_blocks = blocks;
    m->cfg_edges = edges;
    m->post_opt_ir_instructions = instrs;
}

void compiler_metrics_set_spill_total(CompilerMetrics *m, void *ra_results) {
    RegAllocResult **ra = (RegAllocResult **)ra_results;
    if (!m || !ra) return;
    
    int total_spilled = 0;
    int total_loads = 0;
    int total_stores = 0;
    
    int func_count = 0;
    for (int i = 0; ra[i]; i++) func_count++;
    
    if (func_count > 0) {
        m->func_spill_stats = calloc(func_count, sizeof(FunctionSpillStats));
        m->func_spill_count = func_count;
    }
    
    for (int i = 0; ra[i]; i++) {
        int spilled = 0;
        for (int j = 0; j < ra[i]->var_count; j++) {
            if (ra[i]->reg_index[j] < 0)
                spilled++;
        }
        
        total_spilled += spilled;
        total_loads += ra[i]->spill_loads;
        total_stores += ra[i]->spill_stores;
        
        if (m->func_spill_stats) {
            m->func_spill_stats[i].func_name = strdup(ra[i]->func_name);
            m->func_spill_stats[i].spilled_variables = spilled;
            m->func_spill_stats[i].spill_loads = ra[i]->spill_loads;
            m->func_spill_stats[i].spill_stores = ra[i]->spill_stores;
        }
    }
    
    m->total_spilled_variables = total_spilled;
    m->total_spill_loads = total_loads;
    m->total_spill_stores = total_stores;
    
    /* Calculate max pressure and IG nodes across all functions */
    int max_press = 0;
    int ig_nodes = 0;
    for (int i = 0; ra[i]; i++) {
        if (ra[i]->max_reg_pressure > max_press) max_press = ra[i]->max_reg_pressure;
        ig_nodes += ra[i]->ig_node_count;
    }
    m->max_register_pressure = max_press;
    m->interference_graph_nodes = ig_nodes;
}

void compiler_metrics_read_assembly_lines(CompilerMetrics *m, const char *asm_path) {
    if (!m || !asm_path) return;
    FILE *fp = fopen(asm_path, "r");
    if (!fp) return;
    int lines = 0;
    char buf[512];
    while (fgets(buf, sizeof buf, fp)) {
        char *p = buf;
        while (*p == ' ' || *p == '\t') p++;
        if (*p == '\n' || *p == '\r' || *p == '\0') continue;
        if (*p == '#') continue;
        lines++;
    }
    fclose(fp);
    m->assembly_nonblank_lines = lines;
}

void compiler_metrics_analyze_assembly(CompilerMetrics *m, const char *asm_path) {
    if (!m || !asm_path) return;
    FILE *fp = fopen(asm_path, "r");
    if (!fp) return;
    
    int total = 0, loads = 0, stores = 0, alu = 0, branches = 0;
    char buf[512];
    
    while (fgets(buf, sizeof buf, fp)) {
        char *p = buf;
        while (*p == ' ' || *p == '\t') p++;
        if (*p == '\n' || *p == '\r' || *p == '\0' || *p == '#') continue;
        
        total++;
        
        char *start = p;
        while (*p && !isspace(*p) && *p != '.') p++;
        int len = p - start;
        if (len == 0) continue;
        char mnem[16];
        strncpy(mnem, start, len < 15 ? len : 15);
        mnem[len < 15 ? len : 15] = '\0';
        
        if (strcmp(mnem, "lw") == 0 || strcmp(mnem, "ld") == 0 || 
            strcmp(mnem, "lwu") == 0 || strcmp(mnem, "flw") == 0 ||
            strcmp(mnem, "fld") == 0 || strcmp(mnem, "lbu") == 0 ||
            strcmp(mnem, "lhu") == 0 || strcmp(mnem, "lb") == 0 ||
            strcmp(mnem, "lh") == 0)
            loads++;
        else if (strcmp(mnem, "sw") == 0 || strcmp(mnem, "sd") == 0 ||
                 strcmp(mnem, "fsw") == 0 || strcmp(mnem, "fsd") == 0 ||
                 strcmp(mnem, "sb") == 0 || strcmp(mnem, "sh") == 0)
            stores++;
        else if (strcmp(mnem, "j") == 0 || strcmp(mnem, "jal") == 0 ||
                 strcmp(mnem, "jr") == 0 || strcmp(mnem, "jalr") == 0 ||
                 strcmp(mnem, "beq") == 0 || strcmp(mnem, "bne") == 0 ||
                 strcmp(mnem, "blt") == 0 || strcmp(mnem, "bge") == 0 ||
                 strcmp(mnem, "bltu") == 0 || strcmp(mnem, "bgeu") == 0)
            branches++;
        else if (strcmp(mnem, ".align") != 0 && strcmp(mnem, ".globl") != 0 &&
                 strcmp(mnem, ".text") != 0 && strcmp(mnem, ".data") != 0 &&
                 strcmp(mnem, ".string") != 0)
            alu++;
    }
    
    fclose(fp);
    
    m->assembly_total_instructions = total;
    m->assembly_loads = loads;
    m->assembly_stores = stores;
    m->assembly_alu_ops = alu;
    m->assembly_branches = branches;
    
    if (total > 0) {
        m->memory_intensity = (double)(loads + stores) / total;
    }
}

void compiler_metrics_fprint(const CompilerMetrics *m, FILE *fp) {
    if (!m || !fp) return;
    
    fprintf(fp, "=== Compiler Metrics ===\n\n");
    
    fprintf(fp, "-- IR --\n");
    fprintf(fp, "IR instructions (pre):         %d\n", m->pre_opt_ir_instructions);
    fprintf(fp, "IR instructions (post):        %d\n", m->post_opt_ir_instructions);
    fprintf(fp, "Basic blocks:                  %d\n", m->post_opt_basic_blocks);
    fprintf(fp, "CFG edges:                     %d\n", m->cfg_edges);
    fprintf(fp, "Loops:                         %d\n\n", m->loops_found);
    
    fprintf(fp, "-- Optimization --\n");
    fprintf(fp, "DCE removed:                   %d", m->dce_removed_instructions);
    if (m->pre_opt_ir_instructions > 0) {
        int pct = (100 * m->dce_removed_instructions) / m->pre_opt_ir_instructions;
        fprintf(fp, " (%d%%)", pct);
    }
    fprintf(fp, "\n");
    fprintf(fp, "Const folded:                  %d\n", m->const_folded);
    fprintf(fp, "Copy propagated:               %d\n", m->copy_propagated);
    fprintf(fp, "IR growth factor:              %.2f\n\n", m->ir_growth_factor);
    
    fprintf(fp, "-- Register Allocation --\n");
    fprintf(fp, "Spilled variables:             %d\n", m->total_spilled_variables);
    fprintf(fp, "Spill loads:                   %d\n", m->total_spill_loads);
    fprintf(fp, "Spill stores:                  %d\n", m->total_spill_stores);
    fprintf(fp, "Max register pressure:         %d\n", m->max_register_pressure);
    fprintf(fp, "Interference graph nodes:      %d\n", m->interference_graph_nodes);
    fprintf(fp, "Max degree:                    %d\n\n", m->max_register_pressure);
    
    fprintf(fp, "-- Assembly --\n");
    fprintf(fp, "Total instructions:            %d\n", m->assembly_total_instructions);
    fprintf(fp, "Loads:                         %d\n", m->assembly_loads);
    fprintf(fp, "Stores:                        %d\n", m->assembly_stores);
    fprintf(fp, "ALU ops:                       %d\n", m->assembly_alu_ops);
    fprintf(fp, "Branches:                      %d\n", m->assembly_branches);
    fprintf(fp, "Memory intensity:              %.2f\n\n", m->memory_intensity);
    
    fprintf(fp, "-- Runtime --\n");
    fprintf(fp, "Avg execution time:            %.1f ms\n", m->avg_execution_time_ms);
    fprintf(fp, "Dynamic instructions:          %ld\n", m->dynamic_instruction_count);
    fprintf(fp, "Peak memory:                   %ld KB\n\n", m->peak_memory_kb);
    
    fprintf(fp, "==========================\n");
}

void compiler_metrics_print_and_save(const CompilerMetrics *m, const char *path) {
    if (!m) return;
    compiler_metrics_fprint(m, stdout);
    if (!path) return;
    FILE *fp = fopen(path, "w");
    if (!fp) {
        perror("compiler_metrics_print_and_save");
        return;
    }
    compiler_metrics_fprint(m, fp);
    fclose(fp);
}
