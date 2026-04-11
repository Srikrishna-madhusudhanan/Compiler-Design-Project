#include "compiler_metrics.h"
#include "ir_opt.h"
#include "reg_alloc.h"
#include <stdio.h>
#include <string.h>

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

int compiler_metrics_count_basic_blocks(const IRProgram *prog) {
    if (!prog) return 0;
    int n = 0;
    for (IRFunc *f = prog->funcs; f; f = f->next) {
        CFG *cfg = build_cfg(f);
        if (!cfg) continue;
        for (BasicBlock *bb = cfg->blocks; bb; bb = bb->next)
            n++;
        free_cfg(cfg);
    }
    return n;
}

void compiler_metrics_set_spill_total(CompilerMetrics *m, void *ra_results) {
    RegAllocResult **ra = (RegAllocResult **)ra_results;
    if (!m || !ra) return;
    int total = 0;
    for (int i = 0; ra[i]; i++) {
        for (int j = 0; j < ra[i]->var_count; j++) {
            if (ra[i]->reg_index[j] < 0)
                total++;
        }
    }
    m->total_spilled_variables = total;
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

void compiler_metrics_fprint(const CompilerMetrics *m, FILE *fp) {
    if (!m || !fp) return;
    fprintf(fp, "=== Compiler metrics ===\n");
    fprintf(fp, "Code size (IR instructions, pre-opt):    %d\n", m->pre_opt_ir_instructions);
    fprintf(fp, "Code size (IR instructions, post-opt):   %d\n", m->post_opt_ir_instructions);
    fprintf(fp, "Basic blocks (post-optimization, CFG):  %d\n", m->post_opt_basic_blocks);
    fprintf(fp, "DCE removed instructions:               %d\n", m->dce_removed_instructions);
    fprintf(fp, "DCE removed definitions (with result):  %d\n", m->dce_removed_definitions);
    fprintf(fp, "Register pressure (spilled variables):  %d\n", m->total_spilled_variables);
    fprintf(fp, "Assembly non-blank / non-comment lines: %d\n", m->assembly_nonblank_lines);
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
