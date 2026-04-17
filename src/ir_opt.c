#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ir_opt.h"

void optimize_program(IRProgram *prog, OptLevel level, struct CompilerMetrics *metrics) {
    /* No IR optimizations for now */
}

/* Helper to add a successor edge */
static void add_successor(BasicBlock *from, BasicBlock *to) {
    for (int i = 0; i < from->succ_count; i++) {
        if (from->succs[i] == to) return;
    }
    from->succs = realloc(from->succs, sizeof(BasicBlock*) * (from->succ_count + 1));
    from->succs[from->succ_count++] = to;

    to->preds = realloc(to->preds, sizeof(BasicBlock*) * (to->pred_count + 1));
    to->preds[to->pred_count++] = from;
}

CFG* build_cfg(IRFunc *f) {
    if (!f || !f->instrs) return NULL;

    CFG *cfg = calloc(1, sizeof(CFG));
    cfg->func_name = strdup(f->name);

    /* 1. Identify block boundaries */
    /* A block starts at:
       - The first instruction
       - A label
       - The instruction after a branch or return
    */
    int *is_start = calloc(2048, sizeof(int)); // instruction index
    int instr_count = 0;
    for (IRInstr *i = f->instrs; i; i = i->next) instr_count++;

    IRInstr **instr_arr = malloc(sizeof(IRInstr*) * instr_count);
    int idx = 0;
    for (IRInstr *i = f->instrs; i; i = i->next) instr_arr[idx++] = i;

    is_start[0] = 1;
    for (int i = 0; i < instr_count; i++) {
        if (instr_arr[i]->kind == IR_GOTO || instr_arr[i]->kind == IR_IF || instr_arr[i]->kind == IR_RETURN || instr_arr[i]->kind == IR_THROW) {
            if (i + 1 < instr_count) is_start[i + 1] = 1;
        }
        if (instr_arr[i]->kind == IR_LABEL) {
            is_start[i] = 1;
        }
    }

    /* 2. Create blocks */
    BasicBlock *last_bb = NULL;
    int block_id = 0;
    for (int i = 0; i < instr_count; i++) {
        if (is_start[i]) {
            BasicBlock *bb = calloc(1, sizeof(BasicBlock));
            bb->id = block_id++;
            bb->instrs = instr_arr[i];
            if (last_bb) {
                last_bb->next = bb;
                /* By default, fall through if last inst wasn't a GOTO or RETURN */
                if (last_bb->last->kind != IR_GOTO && last_bb->last->kind != IR_RETURN && last_bb->last->kind != IR_THROW) {
                    add_successor(last_bb, bb);
                }
            } else {
                cfg->entry = bb;
                cfg->blocks = bb;
            }
            last_bb = bb;
        }
        last_bb->last = instr_arr[i];
    }
    cfg->block_count = block_id;

    /* 3. Add control flow edges from branches */
    for (BasicBlock *bb = cfg->blocks; bb; bb = bb->next) {
        IRInstr *last = bb->last;
        char *target_label = NULL;
        if (last->kind == IR_GOTO || last->kind == IR_IF || last->kind == IR_TRY_BEGIN) {
            target_label = last->label;
        }
        
        if (target_label) {
            /* Find block starting with this label */
            for (BasicBlock *target_bb = cfg->blocks; target_bb; target_bb = target_bb->next) {
                if (target_bb->instrs->kind == IR_LABEL && strcmp(target_bb->instrs->label, target_label) == 0) {
                    add_successor(bb, target_bb);
                    break;
                }
            }
        }
    }

    free(is_start);
    free(instr_arr);
    return cfg;
}

void free_cfg(CFG *cfg) {
    if (!cfg) return;
    BasicBlock *b = cfg->blocks;
    while (b) {
        BasicBlock *next = b->next;
        if (b->preds) free(b->preds);
        if (b->succs) free(b->succs);
        for(int i=0; i<b->live_in_count; i++) free(b->live_in[i]);
        free(b->live_in);
        for(int i=0; i<b->live_out_count; i++) free(b->live_out[i]);
        free(b->live_out);
        for(int i=0; i<b->use_count; i++) free(b->use[i]);
        free(b->use);
        for(int i=0; i<b->def_count; i++) free(b->def[i]);
        free(b->def);
        free(b);
        b = next;
    }
    if (cfg->func_name) free(cfg->func_name);
    free(cfg);
}

static void add_to_set(char ***set, int *count, const char *name) {
    if (!name) return;
    for (int i = 0; i < *count; i++) {
        if (strcmp((*set)[i], name) == 0) return;
    }
    *set = realloc(*set, sizeof(char*) * (*count + 1));
    (*set)[(*count)++] = strdup(name);
}

static int sets_equal(char **s1, int c1, char **s2, int c2) {
    if (c1 != c2) return 0;
    for (int i = 0; i < c1; i++) {
        int found = 0;
        for (int j = 0; j < c2; j++) {
            if (strcmp(s1[i], s2[j]) == 0) { found = 1; break; }
        }
        if (!found) return 0;
    }
    return 1;
}

void compute_liveness(CFG *cfg) {
    if (!cfg) return;

    /* 1. Compute GEN (use) and KILL (def) per block */
    for (BasicBlock *bb = cfg->blocks; bb; bb = bb->next) {
        for (IRInstr *i = bb->instrs; ; i = i->next) {
            /* Record uses */
            IROperand *uses[4] = {NULL};
            int n_use = 0;
            switch (i->kind) {
                case IR_ASSIGN: uses[n_use++] = &i->src; break;
                case IR_BINOP:  uses[n_use++] = &i->left; uses[n_use++] = &i->right; break;
                case IR_UNOP:   uses[n_use++] = &i->unop_src; break;
                case IR_PARAM:  uses[n_use++] = &i->src; break;
                case IR_RETURN: uses[n_use++] = &i->src; break;
                case IR_IF:     uses[n_use++] = &i->if_left; uses[n_use++] = &i->if_right; break;
                case IR_LOAD:   uses[n_use++] = &i->base; uses[n_use++] = &i->index; break;
                case IR_STORE:  uses[n_use++] = &i->base; uses[n_use++] = &i->index; uses[n_use++] = &i->store_val; break;
                case IR_THROW:  uses[n_use++] = &i->src; break;
                default: break;
            }
            for (int k = 0; k < n_use; k++) {
                if (uses[k] && !uses[k]->is_const && uses[k]->name) {
                    /* If it's defined in this BB before being used, it's not in GEN */
                    int defined = 0;
                    for (int d = 0; d < bb->def_count; d++) {
                        if (strcmp(bb->def[d], uses[k]->name) == 0) { defined = 1; break; }
                    }
                    if (!defined) add_to_set(&bb->use, &bb->use_count, uses[k]->name);
                }
            }
            /* Record defs */
            if (i->result) {
                add_to_set(&bb->def, &bb->def_count, i->result);
            }

            if (i == bb->last) break;
        }
    }

    /* 2. Fixpoint iteration */
    int changed = 1;
    while (changed) {
        changed = 0;
        /* Walk blocks backwards for faster convergence */
        for (BasicBlock *bb = cfg->blocks; bb; bb = bb->next) {
            /* LIVE_OUT[B] = Union(LIVE_IN[S] for S in successors) */
            int old_out_count = bb->live_out_count;
            char **old_out = bb->live_out;
            bb->live_out = NULL;
            bb->live_out_count = 0;
            for (int i = 0; i < bb->succ_count; i++) {
                BasicBlock *s = bb->succs[i];
                for (int j = 0; j < s->live_in_count; j++) {
                    add_to_set(&bb->live_out, &bb->live_out_count, s->live_in[j]);
                }
            }
            if (!sets_equal(old_out, old_out_count, bb->live_out, bb->live_out_count)) changed = 1;
            for(int i=0; i<old_out_count; i++) free(old_out[i]);
            free(old_out);

            /* LIVE_IN[B] = GEN[B] Union (LIVE_OUT[B] - KILL[B]) */
            int old_in_count = bb->live_in_count;
            char **old_in = bb->live_in;
            bb->live_in = NULL;
            bb->live_in_count = 0;
            for (int i = 0; i < bb->use_count; i++) add_to_set(&bb->live_in, &bb->live_in_count, bb->use[i]);
            for (int i = 0; i < bb->live_out_count; i++) {
                int killed = 0;
                for (int j = 0; j < bb->def_count; j++) {
                    if (strcmp(bb->def[j], bb->live_out[i]) == 0) { killed = 1; break; }
                }
                if (!killed) add_to_set(&bb->live_in, &bb->live_in_count, bb->live_out[i]);
            }
            if (!sets_equal(old_in, old_in_count, bb->live_in, bb->live_in_count)) changed = 1;
            for(int i=0; i<old_in_count; i++) free(old_in[i]);
            free(old_in);
        }
    }
}

void export_cfg_to_json(CFG *cfg, const char *path) {
    if (!cfg || !path) return;
    FILE *fp = fopen(path, "w");
    if (!fp) return;

    fprintf(fp, "{\n");
    fprintf(fp, "  \"func_name\": \"%s\",\n", cfg->func_name);
    fprintf(fp, "  \"blocks\": [\n");
    for (BasicBlock *bb = cfg->blocks; bb; bb = bb->next) {
        fprintf(fp, "    {\n");
        fprintf(fp, "      \"id\": %d,\n", bb->id);
        fprintf(fp, "      \"instrs\": [\n");
        for (IRInstr *i = bb->instrs; ; i = i->next) {
            char buf[128];
            ir_snprint_instr(buf, sizeof(buf), i);
            fprintf(fp, "        \"%s\"%s\n", buf, (i == bb->last) ? "" : ",");
            if (i == bb->last) break;
        }
        fprintf(fp, "      ],\n");
        fprintf(fp, "      \"successors\": [");
        for (int j = 0; j < bb->succ_count; j++) {
            fprintf(fp, "%d%s", bb->succs[j]->id, (j == bb->succ_count - 1) ? "" : ", ");
        }
        fprintf(fp, "]\n");
        fprintf(fp, "    }%s\n", bb->next ? "," : "");
    }
    fprintf(fp, "  ]\n");
    fprintf(fp, "}\n");
    fclose(fp);
}

void eliminate_unreachable_blocks(CFG *cfg) {
    /* Stub */
}
