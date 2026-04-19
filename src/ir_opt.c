#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "ir_opt.h"
#include "compiler_metrics.h"
#include "y.tab.h"

/* --- CFG Construction --- */

static BasicBlock* create_bb(int id) {
    BasicBlock *bb = calloc(1, sizeof(BasicBlock));
    bb->id = id;
    return bb;
}

static void add_succ(BasicBlock *src, BasicBlock *dest) {
    if (!src || !dest) return;
    for (int i = 0; i < src->succ_count; i++) {
        if (src->succs[i] == dest) return;
    }
    src->succs = realloc(src->succs, sizeof(BasicBlock*) * (src->succ_count + 1));
    src->succs[src->succ_count++] = dest;

    dest->preds = realloc(dest->preds, sizeof(BasicBlock*) * (dest->pred_count + 1));
    dest->preds[dest->pred_count++] = src;
}

static BasicBlock* find_bb_by_label(BasicBlock *list, const char *label) {
    while (list) {
        if (list->instrs && list->instrs->kind == IR_LABEL && strcmp(list->instrs->label, label) == 0) {
            return list;
        }
        list = list->next;
    }
    return NULL;
}

CFG* build_cfg(IRFunc *f) {
    if (!f || !f->instrs) return NULL;

    CFG *cfg = calloc(1, sizeof(CFG));
    cfg->func_name = strdup(f->name);

    BasicBlock *head = NULL, *tail = NULL;
    int bb_count = 0;

    IRInstr *curr = f->instrs;
    while (curr) {
        BasicBlock *new_bb = create_bb(bb_count++);
        new_bb->instrs = curr;

        if (!head) head = new_bb;
        if (tail) tail->next = new_bb;
        tail = new_bb;

        while (curr) {
            new_bb->last = curr;
            if (curr->kind == IR_GOTO || curr->kind == IR_IF || curr->kind == IR_RETURN || curr->kind == IR_TRY_BEGIN || curr->kind == IR_THROW) {
                curr = curr->next;
                break;
            }
            if (curr->next && curr->next->kind == IR_LABEL) {
                curr = curr->next;
                break;
            }
            curr = curr->next;
        }
    }
    cfg->blocks = head;
    cfg->entry = head;
    cfg->block_count = bb_count;

    BasicBlock *bb = head;
    while (bb) {
        IRInstr *last = bb->last;
        if (last->kind == IR_GOTO) {
            BasicBlock *target = find_bb_by_label(head, last->label);
            if (target) add_succ(bb, target);
        } else if (last->kind == IR_IF || last->kind == IR_TRY_BEGIN) {
            BasicBlock *target = find_bb_by_label(head, last->label);
            if (target) add_succ(bb, target);
            if (bb->next) add_succ(bb, bb->next);
        } else if (last->kind != IR_RETURN && last->kind != IR_THROW) {
            if (bb->next) add_succ(bb, bb->next);
        }
        bb = bb->next;
    }

    return cfg;
}

void export_cfg_to_json(CFG *cfg, const char *path) {
    if (!cfg || !path) return;
    FILE *fp = fopen(path, "w");
    if (!fp) return;

    fprintf(fp, "{\n");
    fprintf(fp, "  \"func_name\": \"%s\",\n", cfg->func_name);
    fprintf(fp, "  \"blocks\": [\n");
    BasicBlock *bb = cfg->blocks;
    while (bb) {
        fprintf(fp, "    {\n");
        fprintf(fp, "      \"id\": %d,\n", bb->id);
        fprintf(fp, "      \"instrs\": [\n");
        IRInstr *instr = bb->instrs;
        while (instr) {
            char buf[256];
            ir_snprint_instr(buf, sizeof(buf), instr);
            fprintf(fp, "        \"%s\"%s\n", buf, (instr == bb->last) ? "" : ",");
            if (instr == bb->last) break;
            instr = instr->next;
        }
        fprintf(fp, "      ],\n");
        fprintf(fp, "      \"successors\": [");
        for (int i = 0; i < bb->succ_count; i++) {
            fprintf(fp, "%d%s", bb->succs[i]->id, (i == bb->succ_count - 1) ? "" : ", ");
        }
        fprintf(fp, "]\n");
        fprintf(fp, "    }%s\n", (bb->next) ? "," : "");
        bb = bb->next;
    }
    fprintf(fp, "  ]\n");
    fprintf(fp, "}\n");
    fclose(fp);
}

void free_cfg(CFG *cfg) {
    if (!cfg) return;
    BasicBlock *bb = cfg->blocks;
    while (bb) {
        BasicBlock *next = bb->next;
        if (bb->preds) free(bb->preds);
        if (bb->succs) free(bb->succs);
        if (bb->doms)  free(bb->doms);
        if (bb->df)    free(bb->df);
        free(bb);
        bb = next;
    }
    free(cfg->func_name);
    free(cfg);
}

static void mark_reachable(BasicBlock *bb, int *reachable) {
    if (!bb || reachable[bb->id]) return;
    reachable[bb->id] = 1;
    for (int i = 0; i < bb->succ_count; i++) {
        mark_reachable(bb->succs[i], reachable);
    }
}

IRInstr* flatten_cfg(CFG *cfg) {
    if (!cfg || !cfg->blocks) return NULL;

    IRInstr *head = NULL, *tail = NULL;
    BasicBlock *bb = cfg->blocks;

    while (bb) {
        IRInstr *instr = bb->instrs;
        while (instr) {
            IRInstr *next_in_instr_list = instr->next;
            instr->next = NULL;
            if (!head) head = instr;
            if (tail) tail->next = instr;
            tail = instr;

            if (instr == bb->last) break;
            instr = next_in_instr_list;
        }
        bb = bb->next;
    }
    return head;
}


/* --- Control Flow Helpers --- */

static IRRelop negate_relop(IRRelop relop) {
    switch (relop) {
        case IR_LT: return IR_GE;
        case IR_GT: return IR_LE;
        case IR_LE: return IR_GT;
        case IR_GE: return IR_LT;
        case IR_EQ: return IR_NE;
        case IR_NE: return IR_EQ;
        default: return relop;
    }
}

static void free_instr_single(IRInstr *instr) {
    if (!instr) return;
    /* We ONLY free the instruction node. We DO NOT free the strings inside it, 
       because the frontend uses shared pointers directly from the Symbol Table. */
    free(instr);
}

static int eval_relop(int l, int r, IRRelop op) {
    switch (op) {
        case IR_LT: return l < r;
        case IR_GT: return l > r;
        case IR_LE: return l <= r;
        case IR_GE: return l >= r;
        case IR_EQ: return l == r;
        case IR_NE: return l != r;
        default: return 0;
    }
}

static IRInstr* simplify_control_flow(IRInstr *head) {
    if (!head) return NULL;
    int changed = 1;
    int iterations = 0;
    while (changed && iterations < 20) {
        changed = 0;
        iterations++;
        IRInstr **curr_ptr = &head;
        while (*curr_ptr) {
            IRInstr *curr = *curr_ptr;
            if (curr->kind == IR_IF && curr->if_left.is_const && curr->if_right.is_const) {
                if (eval_relop(curr->if_left.const_val, curr->if_right.const_val, curr->relop)) {
                    char *lbl = curr->label;
                    curr->if_left.name = NULL;
                    curr->if_right.name = NULL;
                    curr->kind = IR_GOTO;
                    curr->label = lbl;
                } else {
                    *curr_ptr = curr->next;
                    curr->next = NULL;
                    free_instr_single(curr);
                    changed = 1;
                    continue;
                }
                changed = 1;
            }

            if (curr->kind == IR_GOTO || curr->kind == IR_RETURN || curr->kind == IR_THROW) {
                while (curr->next && curr->next->kind != IR_LABEL) {
                    IRInstr *to_del = curr->next;
                    curr->next = to_del->next;
                    to_del->next = NULL;
                    free_instr_single(to_del);
                    changed = 1;
                }
            }

            /* NOTE: Disabled IF-inversion rewrite (if cond goto L1; goto L2; L1: ...)
             * because it can silently change then/else behavior in non-trivial layouts.
             * Keep conditional control flow untouched unless proven safe by CFG analysis. */

            if (curr->kind == IR_GOTO && curr->next && curr->next->kind == IR_LABEL &&
                strcmp(curr->label, curr->next->label) == 0) {

                *curr_ptr = curr->next;
                curr->next = NULL;
                free_instr_single(curr);
                changed = 1;
                continue;
            }

            if (curr->kind == IR_GOTO) {
                IRInstr *target = head;
                while (target) {
                    if (target->kind == IR_LABEL && strcmp(target->label, curr->label) == 0) {
                        if (target->next && target->next->kind == IR_GOTO) {
                            if (strcmp(curr->label, target->next->label) != 0) {
                                free(curr->label);
                                curr->label = target->next->label ? strdup(target->next->label) : NULL;
                                changed = 1;
                            }
                        }
                        break;
                    }
                    target = target->next;
                }
            }
            curr_ptr = &((*curr_ptr)->next);
        }
    }
    return head;
}

/* --- Local Optimizations (BB Scope) --- */

static void convert_to_assign(IRInstr *instr, IROperand new_src) {
    /* Copy new_src before dropping old operands (new_src may alias instr->left). */
    IROperand src_owned = ir_op_copy(&new_src);
    if (instr->kind == IR_BINOP) {
        ir_free_operand(&instr->left);
        ir_free_operand(&instr->right);
    } else if (instr->kind == IR_UNOP) {
        ir_free_operand(&instr->unop_src);
    }
    instr->kind = IR_ASSIGN;
    instr->src = src_owned;
}

static int fold_unop(IRInstr *instr) {
    if (instr->kind != IR_UNOP) return 0;
    if (instr->unop_src.is_const) {
        int val = 0;
        int valid = 1;
        switch (instr->unop) {
            case '-': val = -instr->unop_src.const_val; break;
            case '!': val = !instr->unop_src.const_val; break;
            default: valid = 0; break;
        }
        if (valid) {
            IROperand const_op; const_op.is_const = 1; const_op.const_val = val; const_op.name = NULL;
            convert_to_assign(instr, const_op);
            return 1;
        }
    }
    return 0;
}

static int fold_constants(IRInstr *instr) {
    if (instr->kind != IR_BINOP) return 0;
    if (instr->left.is_const && instr->right.is_const) {
        int val = 0;
        int valid = 1;
        switch (instr->binop) {
            case '+': val = instr->left.const_val + instr->right.const_val; break;
            case '-': val = instr->left.const_val - instr->right.const_val; break;
            case '*': val = instr->left.const_val * instr->right.const_val; break;
            case '/': 
                if (instr->right.const_val != 0) val = instr->left.const_val / instr->right.const_val; 
                else {
                    printf("Semantic Error: Division by zero detected at line %d\n", instr->line);
                    valid = 0;
                }
                break;
            case '%':
                if (instr->right.const_val != 0) val = instr->left.const_val % instr->right.const_val;
                else {
                    printf("Semantic Error: Modulo by zero detected at line %d\n", instr->line);
                    valid = 0;
                }
                break;
            case '<': val = instr->left.const_val < instr->right.const_val; break;
            case '>': val = instr->left.const_val > instr->right.const_val; break;
            case T_LE: val = instr->left.const_val <= instr->right.const_val; break;
            case T_GE: val = instr->left.const_val >= instr->right.const_val; break;
            case T_EQ: val = instr->left.const_val == instr->right.const_val; break;
            case T_NEQ: val = instr->left.const_val != instr->right.const_val; break;
            case T_AND: val = instr->left.const_val && instr->right.const_val; break;
            case T_OR: val = instr->left.const_val || instr->right.const_val; break;
            default: valid = 0; break;
        }
        if (valid) {
            IROperand const_op; const_op.is_const = 1; const_op.const_val = val; const_op.name = NULL;
            convert_to_assign(instr, const_op);
            return 1;
        }
    }
    return 0;
}

static int peephole_algebraic(IRInstr *instr) {
    if (instr->kind == IR_BINOP) {
        if (instr->binop == '+' ) {
            if (instr->right.is_const && instr->right.const_val == 0) { convert_to_assign(instr, instr->left); return 1; }
            if (instr->left.is_const && instr->left.const_val == 0) { convert_to_assign(instr, instr->right); return 1; }
        }
        if (instr->binop == '-') {
             if (instr->right.is_const && instr->right.const_val == 0) { convert_to_assign(instr, instr->left); return 1; }
            if (!instr->left.is_const && !instr->right.is_const && instr->left.name && instr->right.name &&
                strcmp(instr->left.name, instr->right.name) == 0) {
                IROperand const_op; const_op.is_const = 1; const_op.const_val = 0; const_op.name = NULL;
                convert_to_assign(instr, const_op); return 1;
            }
        }
        if (instr->binop == '*') {
            if (instr->right.is_const && instr->right.const_val == 1) { convert_to_assign(instr, instr->left); return 1; }
            if (instr->left.is_const && instr->left.const_val == 1) { convert_to_assign(instr, instr->right); return 1; }
            if ((instr->right.is_const && instr->right.const_val == 0) || (instr->left.is_const && instr->left.const_val == 0)) {
                IROperand const_op; const_op.is_const = 1; const_op.const_val = 0; const_op.name = NULL;
                convert_to_assign(instr, const_op); return 1;
            }
        }
        if (instr->binop == '/') {
            if (instr->right.is_const && instr->right.const_val == 1) { convert_to_assign(instr, instr->left); return 1; }
            if (!instr->left.is_const && !instr->right.is_const && instr->left.name && instr->right.name &&
                strcmp(instr->left.name, instr->right.name) == 0) {
                IROperand const_op; const_op.is_const = 1; const_op.const_val = 1; const_op.name = NULL;
                convert_to_assign(instr, const_op); return 1;
            }
        }
    }
    return 0;
}

static int strength_reduction(IRInstr *instr) {
    if (instr->kind == IR_BINOP && instr->binop == '*') {
        if (instr->right.is_const && instr->right.const_val == 2) {
            instr->binop = '+';
            instr->right = ir_op_copy(&instr->left);
            return 1;
        } else if (instr->left.is_const && instr->left.const_val == 2) {
            instr->binop = '+';
            instr->left = ir_op_copy(&instr->right);
            return 1;
        }
    }
    return 0;
}

typedef struct ConstVar { char *name; int val; struct ConstVar *next; } ConstVar;
typedef struct CopyVar { char *dest; char *src; struct CopyVar *next; } CopyVar;
typedef struct ExprNode { char *res; IROperand l; IROperand r; int op; struct ExprNode *next; } ExprNode;

static void add_const(ConstVar **list, const char *name, int val) {
    ConstVar *cv = malloc(sizeof(ConstVar)); cv->name = strdup(name); cv->val = val; cv->next = *list; *list = cv;
}
static void add_copy(CopyVar **list, const char *dest, const char *src) {
    CopyVar *cv = malloc(sizeof(CopyVar)); cv->dest = strdup(dest); cv->src = strdup(src); cv->next = *list; *list = cv;
}
static void add_expr(ExprNode **list, const char *res, IROperand l, IROperand r, int op) {
    ExprNode *e = malloc(sizeof(ExprNode)); 
    e->res = strdup(res); 
    e->l = l; if (l.name) e->l.name = strdup(l.name);
    e->r = r; if (r.name) e->r.name = strdup(r.name);
    e->op = op; 
    e->next = *list; 
    *list = e;
}

static int get_const(ConstVar *list, const char *name, int *val) {
    while (list) { if (strcmp(list->name, name) == 0) { *val = list->val; return 1; } list = list->next; } return 0;
}
static const char* get_copy(CopyVar *list, const char *name) {
    while (list) { if (strcmp(list->dest, name) == 0) return list->src; list = list->next; } return NULL;
}

static void remove_const(ConstVar **list, const char *name) {
    if (!list) return;
    ConstVar **curr = list;
    while (*curr) {
        if (strcmp((*curr)->name, name) == 0) { ConstVar *tmp = *curr; *curr = (*curr)->next; free(tmp->name); free(tmp); return; }
        curr = &((*curr)->next);
    }
}

static void invalidate_copies_and_exprs(CopyVar **copies, ExprNode **exprs, const char *name) {
    if (!name) return;
    if (copies) {
        CopyVar **c = copies;
        while (*c) {
            if (strcmp((*c)->dest, name) == 0 || strcmp((*c)->src, name) == 0) {
                CopyVar *tmp = *c; *c = (*c)->next; free(tmp->dest); free(tmp->src); free(tmp);
            } else { c = &((*c)->next); }
        }
    }
    if (exprs) {
        ExprNode **e = exprs;
        while (*e) {
            if (strcmp((*e)->res, name) == 0 || ((*e)->l.name && strcmp((*e)->l.name, name) == 0) || ((*e)->r.name && strcmp((*e)->r.name, name) == 0)) {
                ExprNode *tmp = *e; *e = (*e)->next;
                free(tmp->res);
                if (tmp->l.name) free(tmp->l.name);
                if (tmp->r.name) free(tmp->r.name);
                free(tmp);
            } else { e = &((*e)->next); }
        }
    }
}

static void clear_local_structs(ConstVar *c_list, CopyVar *cp_list, ExprNode *e_list) {
    while (c_list) { ConstVar *tmp = c_list; c_list = c_list->next; free(tmp->name); free(tmp); }
    while (cp_list) { CopyVar *tmp = cp_list; cp_list = cp_list->next; free(tmp->dest); free(tmp->src); free(tmp); }
    while (e_list) {
        ExprNode *tmp = e_list; e_list = e_list->next;
        free(tmp->res);
        if (tmp->l.name) free(tmp->l.name);
        if (tmp->r.name) free(tmp->r.name);
        free(tmp);
    }
}

static int propagate_constants_and_copies(IRInstr *instr, ConstVar **consts, CopyVar **copies) {
    int changed = 0;
    IROperand *ops[5] = {NULL}; int num_ops = 0;

    if (instr->kind == IR_ASSIGN) { ops[0] = &instr->src; num_ops = 1; }
    else if (instr->kind == IR_BINOP) { ops[0] = &instr->left; ops[1] = &instr->right; num_ops = 2; }
    else if (instr->kind == IR_UNOP) { ops[0] = &instr->unop_src; num_ops = 1; }
    else if (instr->kind == IR_IF) { ops[0] = &instr->if_left; ops[1] = &instr->if_right; num_ops = 2; }
    else if (instr->kind == IR_RETURN || instr->kind == IR_THROW) { ops[0] = &instr->src; num_ops = 1; }
    else if (instr->kind == IR_PARAM) { ops[0] = &instr->src; num_ops = 1; }
    else if (instr->kind == IR_LOAD) { ops[0] = &instr->base; ops[1] = &instr->index; num_ops = 2; }
    else if (instr->kind == IR_STORE) { ops[0] = &instr->base; ops[1] = &instr->index; ops[2] = &instr->store_val; num_ops = 3; }
    else if (instr->kind == IR_ALLOCA) { ops[0] = &instr->src; num_ops = 1; }
    else if (instr->kind == IR_CALL_INDIRECT) { ops[0] = &instr->base; num_ops = 1; }

    for (int i = 0; i < num_ops; i++) {
        if (ops[i] && !ops[i]->is_const && ops[i]->name) {
            int val; const char *cpy;
            if (get_const(*consts, ops[i]->name, &val)) {
                if (ops[i]->name) free(ops[i]->name);
                ops[i]->is_const = 1;
                ops[i]->const_val = val;
                ops[i]->name = NULL;
                changed = 1;
            } else if ((cpy = get_copy(*copies, ops[i]->name)) != NULL) {
                if (ops[i]->name) free(ops[i]->name);
                ops[i]->name = strdup(cpy);
                ops[i]->is_const = 0;
                ops[i]->const_val = 0;
                changed = 1;
            }
        }
    }

    if (instr->result) {
        remove_const(consts, instr->result);
        invalidate_copies_and_exprs(copies, NULL, instr->result);

        if (instr->kind == IR_ASSIGN) {
            if (instr->src.is_const) add_const(consts, instr->result, instr->src.const_val);
            else if (instr->src.name) add_copy(copies, instr->result, instr->src.name);
        }
    }
    return changed;
}

static int eliminate_cse(IRInstr *instr, ExprNode **exprs) {
    if (instr->kind != IR_BINOP) {
        if (instr->result) invalidate_copies_and_exprs(NULL, exprs, instr->result);
        return 0;
    }

    ExprNode *e = *exprs;
    while (e) {
        if (e->op == instr->binop) {
            int left_match = (e->l.is_const && instr->left.is_const && e->l.const_val == instr->left.const_val) ||
                             (!e->l.is_const && !instr->left.is_const && e->l.name && instr->left.name && strcmp(e->l.name, instr->left.name) == 0);
            int right_match = (e->r.is_const && instr->right.is_const && e->r.const_val == instr->right.const_val) ||
                              (!e->r.is_const && !instr->right.is_const && e->r.name && instr->right.name && strcmp(e->r.name, instr->right.name) == 0);

            int match = left_match && right_match;
            if (!match) {
                int commutative = (instr->binop == '+' || instr->binop == '*' || instr->binop == T_EQ || instr->binop == T_NEQ);
                if (commutative) {
                    int left_swap = (e->l.is_const && instr->right.is_const && e->l.const_val == instr->right.const_val) ||
                                    (!e->l.is_const && !instr->right.is_const && e->l.name && instr->right.name && strcmp(e->l.name, instr->right.name) == 0);
                    int right_swap = (e->r.is_const && instr->left.is_const && e->r.const_val == instr->left.const_val) ||
                                     (!e->r.is_const && !instr->left.is_const && e->r.name && instr->left.name && strcmp(e->r.name, instr->left.name) == 0);
                    if (left_swap && right_swap) match = 1;
                }
            }

            if (match) {
                IROperand src; src.is_const = 0; src.name = strdup(e->res); src.const_val = 0;
                convert_to_assign(instr, src);
                return 1;
            }
        }
        e = e->next;
    }
    if (instr->result) {
        invalidate_copies_and_exprs(NULL, exprs, instr->result);
        add_expr(exprs, instr->result, instr->left, instr->right, instr->binop);
    }
    return 0;
}

typedef struct StoreRecord {
    char *base;
    char *index;
    int has_const_index;
    int const_index_val;
    struct StoreRecord *next;
} StoreRecord;

static void eliminate_dead_stores_local(BasicBlock *bb) {
    int count = 0;
    IRInstr *cur = bb->instrs;
    while (cur) { count++; if (cur == bb->last) break; cur = cur->next; }
    if (count == 0) return;

    IRInstr **arr = malloc(sizeof(IRInstr*) * count);
    int *keep = malloc(sizeof(int) * count);
    cur = bb->instrs;
    for (int i = 0; i < count; i++) { arr[i] = cur; keep[i] = 1; cur = cur->next; }

    StoreRecord *stores = NULL;

    for (int i = count - 1; i >= 0; i--) {
        IRInstr *instr = arr[i];

        if (instr->kind == IR_STORE) {
            int is_dead = 0;
            StoreRecord *s = stores;
            while (s) {
                if (s->base && instr->base.name && strcmp(s->base, instr->base.name) == 0) {
                    if (instr->index.is_const && s->has_const_index) {
                        if (instr->index.const_val == s->const_index_val) {
                            is_dead = 1; break;
                        }
                    } else if (!instr->index.is_const && !s->has_const_index &&
                               instr->index.name && s->index &&
                               strcmp(instr->index.name, s->index) == 0) {
                        is_dead = 1; break;
                    }
                }
                s = s->next;
            }

            if (is_dead) {
                keep[i] = 0;
                continue;
            } else {
                StoreRecord *ns = malloc(sizeof(StoreRecord));
                ns->base = instr->base.name ? strdup(instr->base.name) : NULL;
                ns->has_const_index = instr->index.is_const;
                ns->const_index_val = instr->index.is_const ? instr->index.const_val : 0;
                ns->index = (instr->index.is_const || !instr->index.name) ? NULL : strdup(instr->index.name);
                ns->next = stores;
                stores = ns;
            }
        } else if (instr->kind == IR_LOAD) {
            StoreRecord **s = &stores;
            while (*s) {
                if ((*s)->base && instr->base.name && strcmp((*s)->base, instr->base.name) == 0) {
                    StoreRecord *tmp = *s;
                    *s = (*s)->next;
                    if (tmp->base) free(tmp->base);
                    if (tmp->index) free(tmp->index);
                    free(tmp);
                } else {
                    s = &((*s)->next);
                }
            }
        } else if (instr->kind == IR_CALL || instr->kind == IR_CALL_INDIRECT) {
            while (stores) {
                StoreRecord *tmp = stores;
                stores = stores->next;
                if (tmp->base) free(tmp->base);
                if (tmp->index) free(tmp->index);
                free(tmp);
            }
        }
    }

    IRInstr *new_head = NULL, *new_tail = NULL;
    for (int i = 0; i < count; i++) {
        if (keep[i]) {
            if (!new_head) new_head = arr[i];
            if (new_tail) new_tail->next = arr[i];
            new_tail = arr[i];
        } else {
            free_instr_single(arr[i]);
        }
    }
    if (new_tail) new_tail->next = NULL;
    bb->instrs = new_head;
    bb->last = new_tail;

    free(keep);
    free(arr);
    while (stores) {
        StoreRecord *tmp = stores; stores = stores->next;
        if (tmp->base) free(tmp->base);
        if (tmp->index) free(tmp->index);
        free(tmp);
    }
}

static void optimize_bb(BasicBlock *bb) {
    int changed = 1;
    while (changed) {
        changed = 0;
        ConstVar *consts = NULL; CopyVar *copies = NULL; ExprNode *exprs = NULL;
        IRInstr *curr = bb->instrs;
        while (curr) {
            changed |= propagate_constants_and_copies(curr, &consts, &copies);
            changed |= eliminate_cse(curr, &exprs);
            changed |= fold_constants(curr);
            changed |= fold_unop(curr);
            changed |= strength_reduction(curr);
            changed |= peephole_algebraic(curr);
            if (curr == bb->last) break;
            curr = curr->next;
        }
        clear_local_structs(consts, copies, exprs);
    }
    eliminate_dead_stores_local(bb);
}

/* --- Liveness Analysis --- */

static int set_contains(char **set, int count, const char *name) {
    if (!name) return 0;
    for (int i = 0; i < count; i++) {
        if (strcmp(set[i], name) == 0) return 1;
    }
    return 0;
}

static void set_add(char ***set, int *count, const char *name) {
    if (!name || set_contains(*set, *count, name)) return;
    *set = realloc(*set, sizeof(char*) * (*count + 1));
    (*set)[(*count)++] = strdup(name);
}

static int set_union(char ***dest, int *dest_count, char **src, int src_count) {
    int changed = 0;
    for (int i = 0; i < src_count; i++) {
        if (!set_contains(*dest, *dest_count, src[i])) {
            set_add(dest, dest_count, src[i]);
            changed = 1;
        }
    }
    return changed;
}

static void set_free(char **set, int count) {
    for (int i = 0; i < count; i++) free(set[i]);
    if (set) free(set);
}

static void compute_use_def(BasicBlock *bb) {
    IRInstr *curr = bb->instrs;
    while (curr) {
        IROperand *ops[5] = {NULL};
        int num_ops = 0;

        if (curr->kind == IR_ASSIGN) { ops[0] = &curr->src; num_ops = 1; }
        else if (curr->kind == IR_BINOP) { ops[0] = &curr->left; ops[1] = &curr->right; num_ops = 2; }
        else if (curr->kind == IR_UNOP) { ops[0] = &curr->unop_src; num_ops = 1; }
        else if (curr->kind == IR_PARAM) { ops[0] = &curr->src; num_ops = 1; }
        else if (curr->kind == IR_IF) { ops[0] = &curr->if_left; ops[1] = &curr->if_right; num_ops = 2; }
        else if (curr->kind == IR_RETURN || curr->kind == IR_THROW) { ops[0] = &curr->src; num_ops = 1; }
        else if (curr->kind == IR_LOAD) { ops[0] = &curr->base; ops[1] = &curr->index; num_ops = 2; }
        else if (curr->kind == IR_STORE) { ops[0] = &curr->base; ops[1] = &curr->index; ops[2] = &curr->store_val; num_ops = 3; }
        else if (curr->kind == IR_ALLOCA) { ops[0] = &curr->src; num_ops = 1; }
        else if (curr->kind == IR_CALL_INDIRECT) { ops[0] = &curr->base; num_ops = 1; }

        for (int i = 0; i < num_ops; i++) {
            if (ops[i] && !ops[i]->is_const && ops[i]->name) {
                if (!set_contains(bb->def, bb->def_count, ops[i]->name)) {
                    set_add(&bb->use, &bb->use_count, ops[i]->name);
                }
            }
        }

        if (curr->result) {
            if (!set_contains(bb->use, bb->use_count, curr->result)) {
                set_add(&bb->def, &bb->def_count, curr->result);
            }
        }

        if (curr == bb->last) break;
        curr = curr->next;
    }
}

void compute_liveness(CFG *cfg) {
    if (!cfg) return;
    BasicBlock *bb = cfg->blocks;
    while (bb) {
        set_free(bb->use, bb->use_count); bb->use = NULL; bb->use_count = 0;
        set_free(bb->def, bb->def_count); bb->def = NULL; bb->def_count = 0;
        set_free(bb->live_in, bb->live_in_count); bb->live_in = NULL; bb->live_in_count = 0;
        set_free(bb->live_out, bb->live_out_count); bb->live_out = NULL; bb->live_out_count = 0;
        compute_use_def(bb);
        bb = bb->next;
    }

    int changed = 1;
    while (changed) {
        changed = 0;
        bb = cfg->blocks;
        while (bb) {
            for (int i = 0; i < bb->succ_count; i++) {
                changed |= set_union(&bb->live_out, &bb->live_out_count, bb->succs[i]->live_in, bb->succs[i]->live_in_count);
            }
            int old_in_count = bb->live_in_count;
            set_union(&bb->live_in, &bb->live_in_count, bb->use, bb->use_count);
            for (int i = 0; i < bb->live_out_count; i++) {
                if (!set_contains(bb->def, bb->def_count, bb->live_out[i])) {
                    set_add(&bb->live_in, &bb->live_in_count, bb->live_out[i]);
                }
            }
            if (bb->live_in_count != old_in_count) changed = 1;
            bb = bb->next;
        }
    }
}

void eliminate_dead_code(CFG *cfg, CompilerMetrics *metrics) {
    if (!cfg) return;
    compute_liveness(cfg);

    BasicBlock *bb = cfg->blocks;
    while (bb) {
        char **current_live = NULL;
        int current_live_count = 0;
        for (int i = 0; i < bb->live_out_count; i++) set_add(&current_live, &current_live_count, bb->live_out[i]);

         int count = 0;
         IRInstr *cur = bb->instrs;
         while (cur) { count++; if (cur == bb->last) break; cur = cur->next; }
         
         if (count > 0) {
             IRInstr **arr = malloc(sizeof(IRInstr*) * count);
             int *keep = malloc(sizeof(int) * count);
             cur = bb->instrs;
             for (int i = 0; i < count; i++) { arr[i] = cur; keep[i] = 1; cur = cur->next; }

             for (int i = count - 1; i >= 0; i--) {
                 IRInstr *instr = arr[i];

                 if (instr->result && !set_contains(current_live, current_live_count, instr->result)) {
                     if (instr->kind != IR_CALL && instr->kind != IR_CALL_INDIRECT && instr->kind != IR_STORE && instr->kind != IR_ALLOCA && instr->kind != IR_RETURN) {
                         keep[i] = 0;
                         continue;
                     }
                 }

                 if (instr->result) {
                     for (int j = 0; j < current_live_count; j++) {
                         if (strcmp(current_live[j], instr->result) == 0) {
                             free(current_live[j]);
                             current_live[j] = current_live[--current_live_count];
                             break;
                         }
                     }
                 }
                 
                 IROperand *ops[5] = {NULL};
                 int num_ops = 0;
                 if (instr->kind == IR_ASSIGN) { ops[0] = &instr->src; num_ops = 1; }
                 else if (instr->kind == IR_BINOP) { ops[0] = &instr->left; ops[1] = &instr->right; num_ops = 2; }
                 else if (instr->kind == IR_UNOP) { ops[0] = &instr->unop_src; num_ops = 1; }
                 else if (instr->kind == IR_PARAM) { ops[0] = &instr->src; num_ops = 1; }
                 else if (instr->kind == IR_IF) { ops[0] = &instr->if_left; ops[1] = &instr->if_right; num_ops = 2; }
                 else if (instr->kind == IR_RETURN) { ops[0] = &instr->src; num_ops = 1; }
                 else if (instr->kind == IR_LOAD) { ops[0] = &instr->base; ops[1] = &instr->index; num_ops = 2; }
                 else if (instr->kind == IR_STORE) { ops[0] = &instr->base; ops[1] = &instr->index; ops[2] = &instr->store_val; num_ops = 3; }
                 else if (instr->kind == IR_ALLOCA) { ops[0] = &instr->src; num_ops = 1; }
                 else if (instr->kind == IR_CALL_INDIRECT) { ops[0] = &instr->base; num_ops = 1; }

                 for (int j = 0; j < num_ops; j++) {
                     if (ops[j] && !ops[j]->is_const && ops[j]->name) {
                         set_add(&current_live, &current_live_count, ops[j]->name);
                     }
                 }
             }
             
             IRInstr *new_head = NULL, *new_tail = NULL;
             for (int i = 0; i < count; i++) {
                 if (keep[i]) {
                     if (!new_head) new_head = arr[i];
                     if (new_tail) new_tail->next = arr[i];
                     new_tail = arr[i];
                 } else {
                     if (metrics) {
                         metrics->dce_removed_instructions++;
                         if (arr[i]->result)
                             metrics->dce_removed_definitions++;
                     }
                     free_instr_single(arr[i]);
                 }
             }
             if (new_tail) new_tail->next = NULL;
             bb->instrs = new_head;
             bb->last = new_tail;

             free(keep);
             free(arr);
         }
         set_free(current_live, current_live_count);
         bb = bb->next;
    }
}

static void mark_reachable_and_cleanup(CFG *cfg) {
    if (!cfg || !cfg->entry) return;
    int *reachable = calloc(cfg->block_count, sizeof(int));
    mark_reachable(cfg->entry, reachable);

    BasicBlock **curr = &cfg->blocks;
    while (*curr) {
        if (!reachable[(*curr)->id]) {
            BasicBlock *to_delete = *curr;
            *curr = to_delete->next;

            for (int i = 0; i < to_delete->succ_count; i++) {
                BasicBlock *succ = to_delete->succs[i];
                for (int j = 0; j < succ->pred_count; j++) {
                    if (succ->preds[j] == to_delete) {
                        succ->preds[j] = succ->preds[--succ->pred_count];
                        break;
                    }
                }
            }
            
             IRInstr *ins = to_delete->instrs;
             while(ins) {
                 IRInstr *nxt = ins->next;
                 free_instr_single(ins);
                 if (ins == to_delete->last) break;
                 ins = nxt;
             }

             if (to_delete->preds) free(to_delete->preds);
             if (to_delete->succs) free(to_delete->succs);
             free(to_delete);
             /* CRITICAL FIX: DO NOT DECREMENT cfg->block_count. 
                This shrinks array allocations while block IDs stay high, 
                causing catastrophic Out-Of-Bounds writes (malloc corrupted top size)! */
        } else {
            curr = &((*curr)->next);
        }
    }
    free(reachable);
}

void eliminate_unreachable_blocks(CFG *cfg) {
    mark_reachable_and_cleanup(cfg);
}


/* --- Dominator Analysis --- */

void compute_dominators(CFG *cfg) {
    if (!cfg || !cfg->entry) return;
    int n = cfg->block_count;

    BasicBlock *bb = cfg->blocks;
    while (bb) {
        if (bb->doms) free(bb->doms);
        bb->doms = malloc(sizeof(int) * n);
        for (int i = 0; i < n; i++) bb->doms[i] = 1;
        bb = bb->next;
    }

    for (int i = 0; i < n; i++) cfg->entry->doms[i] = 0;
    cfg->entry->doms[cfg->entry->id] = 1;

    int changed = 1;
    while (changed) {
        changed = 0;
        bb = cfg->blocks;
        while (bb) {
            if (bb == cfg->entry) { bb = bb->next; continue; }

            int *new_doms = malloc(sizeof(int) * n);
            for (int i = 0; i < n; i++) new_doms[i] = 1;

            if (bb->pred_count > 0) {
                for (int i = 0; i < bb->pred_count; i++) {
                    BasicBlock *p = bb->preds[i];
                    for (int j = 0; j < n; j++) {
                        if (!p->doms[j]) new_doms[j] = 0;
                    }
                }
            } else {
                for (int i = 0; i < n; i++) new_doms[i] = 0;
            }
            new_doms[bb->id] = 1;

            for (int i = 0; i < n; i++) {
                if (bb->doms[i] != new_doms[i]) {
                    changed = 1;
                    bb->doms[i] = new_doms[i];
                }
            }
            free(new_doms);
            bb = bb->next;
        }
    }
}

/* --- Dominance Frontier Analysis (Phase 1 of SSA) --- */

/*
 * idom_of: Return the block-id of the immediate dominator of `b`.
 * The immediate dominator is the closest strict dominator — i.e., the block
 * that dominates `b` but does not dominate any other dominator of `b`.
 * Returns -1 for the entry block (no immediate dominator).
 *
 * We use the existing doms[] bitset: idom(b) is the dominator d≠b such that
 * no other dominator of b strictly dominates d.
 */
int idom_of(CFG *cfg, BasicBlock *b) {
    if (!cfg || !b || b == cfg->entry) return -1;
    int n = cfg->block_count;

    /* Candidate: dominator d of b where d != b */
    int idom_id = -1;
    BasicBlock *bb = cfg->blocks;
    while (bb) {
        if (bb->id != b->id && b->doms[bb->id]) {
            /* bb dominates b — check if it is dominated by current best */
            if (idom_id == -1) {
                idom_id = bb->id;
            } else {
                /* Keep whichever is strictly dominated by the other */
                BasicBlock *cur_idom = cfg->blocks;
                while (cur_idom && cur_idom->id != idom_id) cur_idom = cur_idom->next;
                /* idom is the CLOSEST dominator. If candidate 'bb' is dominated by 
                 * current best 'idom_id', then 'bb' is closer to 'b'. */
                if (bb->doms[idom_id]) {
                    idom_id = bb->id;
                }
            }
        }
        bb = bb->next;
    }
    (void)n;
    return idom_id;
}

/*
 * compute_dominance_frontiers: Populate bb->df for every block.
 *
 * Algorithm (Cooper/Harvey/Kennedy):
 *   For each block b with >= 2 predecessors:
 *     For each predecessor p of b:
 *       runner = p
 *       While runner != idom(b):
 *         runner->df[b->id] = 1
 *         runner = idom(runner)
 *
 * Must be called after compute_dominators().
 */
void compute_dominance_frontiers(CFG *cfg) {
    if (!cfg || !cfg->entry) return;
    int n = cfg->block_count;

    /* Allocate / zero-init df bitsets */
    BasicBlock *bb = cfg->blocks;
    while (bb) {
        if (bb->df) free(bb->df);
        bb->df = calloc(n, sizeof(int));
        bb = bb->next;
    }

    bb = cfg->blocks;
    while (bb) {
        if (bb->pred_count >= 2) {
            int idom_id = idom_of(cfg, bb);
            for (int p = 0; p < bb->pred_count; p++) {
                BasicBlock *runner = bb->preds[p];
                while (runner && runner->id != idom_id) {
                    runner->df[bb->id] = 1;
                    int next_idom = idom_of(cfg, runner);
                    if (next_idom < 0) break;
                    /* Find block with id == next_idom */
                    BasicBlock *r2 = cfg->blocks;
                    while (r2 && r2->id != next_idom) r2 = r2->next;
                    runner = r2;
                }
            }
        }
        bb = bb->next;
    }
}

/* ==========================================================================
 * SSA Construction (Phase 2)
 * ==========================================================================
 *
 * Two-step process:
 *   1. insert_phi_functions  — Cytron iterated-DF phi placement
 *   2. rename_variables      — dominator-tree DFS rename
 * ==========================================================================
 */

/* --- Variable inventory helpers --- */

typedef struct VarEntry {
    char *name;
    struct VarEntry *next;
} VarEntry;

/* Add `name` to list if not already present. Returns 1 if added. */
static int varlist_add(VarEntry **list, const char *name) {
    if (!name) return 0;
    for (VarEntry *v = *list; v; v = v->next)
        if (strcmp(v->name, name) == 0) return 0;
    VarEntry *e = malloc(sizeof(VarEntry));
    e->name = strdup(name);
    e->next = *list;
    *list = e;
    return 1;
}

static void varlist_free(VarEntry *list) {
    while (list) { VarEntry *n = list->next; free(list->name); free(list); list = n; }
}

/*
 * collect_all_vars: Walk every instruction in the CFG and collect every
 * variable name that appears as a *definition* (instr->result).
 * We only SSA-rename these — not function names, labels, or constants.
 */
static VarEntry* collect_all_vars(CFG *cfg) {
    VarEntry *vars = NULL;
    BasicBlock *bb = cfg->blocks;
    while (bb) {
        IRInstr *ins = bb->instrs;
        while (ins) {
            if (ins->result)
                varlist_add(&vars, ins->result);
            if (ins == bb->last) break;
            ins = ins->next;
        }
        bb = bb->next;
    }
    return vars;
}

/* --- Block list (simple int bitset worklist) --- */

typedef struct { int *bits; int n; } Bitset;

static Bitset bs_alloc(int n) {
    Bitset b; b.n = n; b.bits = calloc(n, sizeof(int)); return b;
}
static void bs_free(Bitset b)       { free(b.bits); }
static void bs_set(Bitset b, int i) { if (i >= 0 && i < b.n) b.bits[i] = 1; }
static int  bs_test(Bitset b, int i){ return (i >= 0 && i < b.n) ? b.bits[i] : 0; }
static void bs_clear_all(Bitset b)  { memset(b.bits, 0, b.n * sizeof(int)); }

/* --- Step 1: Phi insertion (Cytron et al.) --- */

/*
 * For a given variable `var_name`, find every block that defines it
 * (has an instruction with result == var_name), then propagate phi
 * placements through the iterated dominance frontier.
 */
static void insert_phis_for_var(CFG *cfg, const char *var_name,
                                 Bitset has_already, Bitset ever_on_wl) {
    int n = cfg->block_count;
    bs_clear_all(has_already);
    bs_clear_all(ever_on_wl);

    /* Worklist = all blocks that define var_name */
    BasicBlock **worklist = malloc(n * sizeof(BasicBlock*));
    int wl_top = 0;

    BasicBlock *bb = cfg->blocks;
    while (bb) {
        IRInstr *ins = bb->instrs;
        int defines = 0;
        while (ins) {
            if (ins->result && strcmp(ins->result, var_name) == 0) {
                defines = 1; break;
            }
            if (ins == bb->last) break;
            ins = ins->next;
        }
        if (defines) {
            bs_set(ever_on_wl, bb->id);
            worklist[wl_top++] = bb;
        }
        bb = bb->next;
    }

    while (wl_top > 0) {
        BasicBlock *x = worklist[--wl_top];

        /* For each block y in DF(x) */
        for (int y_id = 0; y_id < n; y_id++) {
            if (!x->df || !x->df[y_id]) continue;

            /* Find block y */
            BasicBlock *y = cfg->blocks;
            while (y && y->id != y_id) y = y->next;
            if (!y) continue;

            if (!bs_test(has_already, y_id)) {
                /* Insert phi for var_name at top of y (after IR_LABEL if present) */
                IRInstr *phi = ir_make_phi((char *)var_name, y->pred_count, y->instrs ? y->instrs->line : 0);
                for (int k = 0; k < y->pred_count; k++)
                    phi->phi_pred_bb[k] = y->preds[k]->id;
                /* phi_args slots remain NULL — filled by rename pass */

                /* Insert after the leading IR_LABEL (if any) */
                if (y->instrs && y->instrs->kind == IR_LABEL) {
                    phi->next = y->instrs->next;
                    y->instrs->next = phi;
                    if (y->instrs == y->last) y->last = phi;
                } else {
                    phi->next = y->instrs;
                    y->instrs = phi;
                    if (!y->last) y->last = phi;
                }

                bs_set(has_already, y_id);

                if (!bs_test(ever_on_wl, y_id)) {
                    bs_set(ever_on_wl, y_id);
                    worklist[wl_top++] = y;
                }
            }
        }
    }
    free(worklist);
}

static void insert_phi_functions(CFG *cfg, VarEntry *vars) {
    int n = cfg->block_count;
    Bitset has_already = bs_alloc(n);
    Bitset ever_on_wl  = bs_alloc(n);

    for (VarEntry *v = vars; v; v = v->next)
        insert_phis_for_var(cfg, v->name, has_already, ever_on_wl);

    bs_free(has_already);
    bs_free(ever_on_wl);
}

/* --- Step 2: Variable renaming (dominator-tree DFS) --- */

/*
 * Per-variable version stack.  Each entry is:
 *   var_name -> stack of current SSA names (top = newest).
 */
typedef struct NameStack {
    char  *orig;       /* original variable name */
    char **stack;      /* stack of SSA names (strdup'd) */
    int    sp;         /* stack pointer (0 = empty) */
    int    cap;
    int    counter;    /* next available version number */
} NameStack;

typedef struct RenameState {
    NameStack *stacks;
    int        nstacks;
    int        cap;
} RenameState;

static NameStack* rs_find(RenameState *rs, const char *name) {
    for (int i = 0; i < rs->nstacks; i++)
        if (strcmp(rs->stacks[i].orig, name) == 0)
            return &rs->stacks[i];
    return NULL;
}

static NameStack* rs_get_or_create(RenameState *rs, const char *name) {
    NameStack *s = rs_find(rs, name);
    if (s) return s;
    if (rs->nstacks == rs->cap) {
        rs->cap = rs->cap ? rs->cap * 2 : 16;
        rs->stacks = realloc(rs->stacks, rs->cap * sizeof(NameStack));
    }
    NameStack *ns = &rs->stacks[rs->nstacks++];
    ns->orig    = strdup(name);
    ns->stack   = calloc(64, sizeof(char*));
    ns->sp      = 0;
    ns->cap     = 64;
    ns->counter = 0;
    return ns;
}

/* Push a newly generated SSA name onto the stack; return it (caller does NOT free). */
static char* rs_push_new(RenameState *rs, const char *orig_name) {
    NameStack *s = rs_get_or_create(rs, orig_name);
    if (s->sp == s->cap) {
        s->cap *= 2;
        s->stack = realloc(s->stack, s->cap * sizeof(char*));
    }
    char buf[128];
    snprintf(buf, sizeof(buf), "%s.%d", orig_name, s->counter++);
    s->stack[s->sp++] = strdup(buf);
    return s->stack[s->sp - 1];
}

/* Peek at the top of the stack (most recent SSA name), or `orig_name` if empty. */
static const char* rs_top(RenameState *rs, const char *orig_name) {
    NameStack *s = rs_find(rs, orig_name);
    if (!s || s->sp == 0) return orig_name;   /* no rename yet — use original */
    return s->stack[s->sp - 1];
}

/* Pop `count` names from the stack. */
static void rs_pop(RenameState *rs, const char *orig_name, int count) {
    NameStack *s = rs_find(rs, orig_name);
    if (!s) return;
    while (count-- > 0 && s->sp > 0) {
        free(s->stack[--s->sp]);
    }
}

static void rs_free(RenameState *rs) {
    for (int i = 0; i < rs->nstacks; i++) {
        free(rs->stacks[i].orig);
        for (int j = 0; j < rs->stacks[i].sp; j++)
            free(rs->stacks[i].stack[j]);
        free(rs->stacks[i].stack);
    }
    free(rs->stacks);
}

/*
 * rename_operand: Replace a use operand's name with the current SSA top.
 * Only renames if the name is a *tracked* variable (present in `vars`).
 */
static void rename_operand(IROperand *op, RenameState *rs, VarEntry *vars) {
    if (!op || op->is_const || !op->name) return;
    /* Only rename names we know about (skip function names / labels) */
    VarEntry *v = vars;
    while (v) {
        if (strcmp(v->name, op->name) == 0) {
            const char *ssa = rs_top(rs, op->name);
            if (ssa != op->name) {  /* actually got a rename */
                free(op->name);
                op->name = strdup(ssa);
            }
            return;
        }
        v = v->next;
    }
}

/*
 * rename_block: The core dominator-tree DFS rename.
 * `push_counts` tracks how many names each block pushes (for pop-on-exit).
 */
static void rename_block(BasicBlock *bb, CFG *cfg, RenameState *rs, VarEntry *vars) {
    /* Track how many pushes this block makes per variable (for cleanup on exit) */
    typedef struct { char *orig; int pushes; } PushRec;
    PushRec *pushed = NULL;
    int pushed_n = 0, pushed_cap = 0;

    #define TRACK_PUSH(orig_name) do { \
        int _found = 0; \
        for (int _i = 0; _i < pushed_n; _i++) \
            if (strcmp(pushed[_i].orig, (orig_name)) == 0) { pushed[_i].pushes++; _found = 1; break; } \
        if (!_found) { \
            if (pushed_n == pushed_cap) { pushed_cap = pushed_cap ? pushed_cap*2 : 8; \
                pushed = realloc(pushed, pushed_cap * sizeof(PushRec)); } \
            pushed[pushed_n].orig = strdup(orig_name); pushed[pushed_n].pushes = 1; pushed_n++; \
        } \
    } while(0)

    IRInstr *ins = bb->instrs;
    while (ins) {
        if (ins->kind == IR_PHI) {
            /* Definition: rename the result of this phi to a new SSA name */
            if (ins->result) {
                char *orig = strdup(ins->result);
                const char *ssa_name = rs_push_new(rs, orig);
                free(ins->result);
                ins->result = strdup(ssa_name);
                TRACK_PUSH(orig);
                free(orig);
            }
        } else {
            /* --- Rename uses first --- */
            switch (ins->kind) {
                case IR_ASSIGN:
                    rename_operand(&ins->src, rs, vars);
                    break;
                case IR_BINOP:
                    rename_operand(&ins->left, rs, vars);
                    rename_operand(&ins->right, rs, vars);
                    break;
                case IR_UNOP:
                    rename_operand(&ins->unop_src, rs, vars);
                    break;
                case IR_PARAM:
                    rename_operand(&ins->src, rs, vars);
                    break;
                case IR_IF:
                    rename_operand(&ins->if_left, rs, vars);
                    rename_operand(&ins->if_right, rs, vars);
                    break;
                case IR_RETURN:
                    rename_operand(&ins->src, rs, vars);
                    break;
                case IR_LOAD:
                    rename_operand(&ins->base, rs, vars);
                    rename_operand(&ins->index, rs, vars);
                    break;
                case IR_STORE:
                    rename_operand(&ins->base, rs, vars);
                    rename_operand(&ins->index, rs, vars);
                    rename_operand(&ins->store_val, rs, vars);
                    break;
                case IR_ALLOCA:
                    rename_operand(&ins->src, rs, vars);
                    break;
                case IR_CALL_INDIRECT:
                    rename_operand(&ins->base, rs, vars);
                    break;
                default:
                    break;
            }

            /* --- Rename definition (push new SSA name) --- */
            if (ins->result) {
                /* Check the result name is a tracked variable */
                VarEntry *v = vars;
                while (v) {
                    if (strcmp(v->name, ins->result) == 0) {
                        char *orig = strdup(ins->result);
                        const char *ssa_name = rs_push_new(rs, orig);
                        free(ins->result);
                        ins->result = strdup(ssa_name);
                        TRACK_PUSH(orig);
                        free(orig);
                        break;
                    }
                    v = v->next;
                }
            }
        }

        if (ins == bb->last) break;
        ins = ins->next;
    }

    /* Fill in phi arguments in successor blocks */
    for (int si = 0; si < bb->succ_count; si++) {
        BasicBlock *succ = bb->succs[si];
        IRInstr *phi = succ->instrs;
        /* Skip initial label if present */
        if (phi && phi->kind == IR_LABEL) phi = phi->next;
        
        /* Step 2.2: Detect backedges
         * A backedge occurs when successor has already been visited (phis are already SSA-renamed).
         * Backedge phis have phi->result with .N suffix already (e.g., sum.7).
         * For backedges, we should use the current renamed state for this block. */
        /* TODO: Use is_backedge for improved backedge handling */
        /* int is_backedge = 0;
        if (phi && phi->kind == IR_PHI) {
            if (phi->result && strchr(phi->result, '.')) {
                is_backedge = 1;
            }
        } */
        
        while (phi && phi->kind == IR_PHI) {
            /* Find which slot in phi corresponds to `bb` */
            for (int k = 0; k < phi->phi_arity; k++) {
                if (phi->phi_pred_bb[k] == bb->id) {
                    /* Step 2.1: Improved base name recovery
                     * phi->result holds the destination SSA name.
                     * If successor was already visited (backedge), phi->result is already 
                     * SSA-renamed (e.g., sum.7). We must strip the .N suffix to find the 
                     * base variable name. */
                    
                    assert(phi->result != NULL && strlen(phi->result) > 0);
                    
                    char orig_var[128];
                    strncpy(orig_var, phi->result, sizeof(orig_var) - 1);
                    orig_var[sizeof(orig_var) - 1] = '\0';
                    
                    /* Strip .N suffix if present (validate format) */
                    char *dot = strchr(orig_var, '.');
                    if (dot) {
                        *dot = '\0';  /* Strip .N suffix */
                    }

                    const char *top = rs_top(rs, orig_var);
                    
                    /* Step 2.1: Fallback handling - if stack lookup fails, use full phi->result */
                    if (!top) {
                        fprintf(stderr, "WARNING: Stack lookup failed for base var '%s' (from phi->result='%s'); using full phi->result as fallback\n",
                                orig_var, phi->result);
                        top = phi->result;
                    }

                    if (phi->phi_args[k]) free(phi->phi_args[k]);
                    phi->phi_args[k] = strdup(top);
                    
                    break;
                }
            }
            if (phi == succ->last) break;
            phi = phi->next;
        }
    }

    /* Step 1.2: Track phi argument filling for debugging
     * After filling phi args for all successors, check if any are still NULL.
     * This helps catch incomplete phi filling due to traversal order issues. */
    for (int si = 0; si < bb->succ_count; si++) {
        BasicBlock *succ = bb->succs[si];
        IRInstr *phi = succ->instrs;
        if (phi && phi->kind == IR_LABEL) phi = phi->next;
        
        while (phi && phi->kind == IR_PHI) {
            for (int k = 0; k < phi->phi_arity; k++) {
                if (phi->phi_pred_bb[k] == bb->id && !phi->phi_args[k]) {
                    /* This should not happen if rename is correct, but log for debugging */
                    fprintf(stderr, "WARNING: Phi in block %d (var '%s') has NULL incoming from block %d\n",
                            succ->id, phi->result ? phi->result : "<unknown>", bb->id);
                }
            }
            if (phi == succ->last) break;
            phi = phi->next;
        }
    }

    /* Recurse into children in the dominator tree */
    BasicBlock *child = cfg->blocks;
    while (child) {
        if (child != bb && child != cfg->entry &&
            idom_of(cfg, child) == bb->id) {
            rename_block(child, cfg, rs, vars);
        }
        child = child->next;
    }

    /* Pop everything this block pushed */
    for (int i = 0; i < pushed_n; i++) {
        rs_pop(rs, pushed[i].orig, pushed[i].pushes);
        free(pushed[i].orig);
    }
    free(pushed);

    #undef TRACK_PUSH
}

static void rename_variables(CFG *cfg, VarEntry *vars) {
    RenameState rs = {0};
    rename_block(cfg->entry, cfg, &rs, vars);
    rs_free(&rs);
}

/* --- Public SSA API --- */

/* Forward declarations for phase 1 validation */
static int validate_phi_args(CFG *cfg);

void ssa_construct(CFG *cfg) {
    if (!cfg || !cfg->entry) return;

    /* Dominators must already be computed by the caller */
    compute_dominance_frontiers(cfg);

    VarEntry *vars = collect_all_vars(cfg);
    if (!vars) return;

    insert_phi_functions(cfg, vars);
    rename_variables(cfg, vars);

    /* Step 1.1: Validate that all phi arguments were properly filled */
    if (validate_phi_args(cfg)) {
        fprintf(stderr, "SSA construction failed: incomplete phi arguments detected\n");
        varlist_free(vars);
        return;
    }

    varlist_free(vars);
}

/* ==========================================================================
 * SSA Deconstruction — Out-of-SSA (Phase 3)
 * ==========================================================================
 *
 * Replace each phi   `x := phi(x_i from pred_i, ...)`   with
 * copy instructions  `x := x_i`   inserted at the end of each pred_i,
 * just before its block terminator (IR_GOTO / IR_IF / IR_RETURN).
 *
 * Then remove all IR_PHI instructions from every block.
 * ==========================================================================
 */

/*
 * insert_copy_before_terminator: Insert `IR_ASSIGN dst := src_name` at the
 * end of `pred_bb`, immediately before its terminator instruction.
 */
static void insert_copy_before_terminator(BasicBlock *pred_bb,
                                           const char *dst,
                                           const char *src_name) {
    IROperand src_op;
    src_op.name      = strdup(src_name);
    src_op.is_const  = 0;
    src_op.const_val = 0;

    IRInstr *copy = ir_make_assign((char *)dst, src_op, pred_bb->last ? pred_bb->last->line : 0);
    free(src_op.name);  /* ir_make_assign strdup's it */

    if (!pred_bb->instrs) {
        /* Empty block — just set */
        pred_bb->instrs = copy;
        pred_bb->last   = copy;
        return;
    }

    IRInstr *term = pred_bb->last;
    if (!term) {
        /* No terminator — append */
        IRInstr *p = pred_bb->instrs;
        while (p->next) p = p->next;
        p->next       = copy;
        pred_bb->last = copy;
        return;
    }

    int is_term = (term->kind == IR_GOTO || term->kind == IR_IF ||
                   term->kind == IR_RETURN);
    if (!is_term) {
        /* Append after last */
        term->next    = copy;
        pred_bb->last = copy;
        return;
    }

    /* Insert before the terminator */
    if (pred_bb->instrs == term) {
        /* Terminator is the only instruction */
        pred_bb->instrs = copy;
        copy->next      = term;
        /* last stays as term */
        return;
    }

    /* Walk to find the instruction before term */
    IRInstr *prev = pred_bb->instrs;
    while (prev->next && prev->next != term) prev = prev->next;
    prev->next = copy;
    copy->next = term;
}

/* ========================================================================== */
/* Phase 4: Parallel Copy Semantics via Temporary Variables */
/* ========================================================================== */

/**
 * gen_phi_temp_for_var: Generate a unique temporary variable name for a specific phi result.
 * This ensures uniqueness by using the phi's destination variable name.
 */
static char *gen_phi_temp_for_var(const char *phi_result, int arg_index) {
    static char buf[128];
    if (phi_result) {
        snprintf(buf, sizeof(buf), "__phi_tmp_%s_%d", phi_result, arg_index);
    } else {
        snprintf(buf, sizeof(buf), "__phi_tmp_arg_%d", arg_index);
    }
    return buf;
}

/* ========================================================================== */
/* Phase 1: Validation Infrastructure */
/* ========================================================================== */

/**
 * validate_phi_args: Check that all phi arguments are properly filled.
 *   Returns 0 if all phis are valid, 1 if any phi has unfilled arguments.
 *   Logs detailed error information for debugging.
 */
static int validate_phi_args(CFG *cfg) {
    int has_errors = 0;

    if (!cfg) return 0;

    BasicBlock *bb = cfg->blocks;
    while (bb) {
        IRInstr *ins = bb->instrs;
        /* Skip optional leading label */
        if (ins && ins->kind == IR_LABEL) ins = ins->next;

        while (ins && ins->kind == IR_PHI) {
            IRInstr *phi = ins;

            /* Check that arity matches predecessor count */
            if (phi->phi_arity != bb->pred_count) {
                fprintf(stderr, "ERROR: Phi in block %d has arity %d but block has %d predecessors\n",
                        bb->id, phi->phi_arity, bb->pred_count);
                has_errors = 1;
            }

            /* Check that all phi_args are non-NULL */
            for (int k = 0; k < phi->phi_arity; k++) {
                if (!phi->phi_args[k]) {
                    fprintf(stderr, "ERROR: Phi in block %d for variable '%s' has NULL incoming value from predecessor %d\n",
                            bb->id, phi->result ? phi->result : "<unknown>", phi->phi_pred_bb[k]);
                    has_errors = 1;
                }

                /* Verify predecessor block ID is valid */
                int found = 0;
                for (int p = 0; p < bb->pred_count; p++) {
                    if (bb->preds[p] && bb->preds[p]->id == phi->phi_pred_bb[k]) {
                        found = 1;
                        break;
                    }
                }
                if (!found) {
                    fprintf(stderr, "ERROR: Phi in block %d references unknown predecessor block ID %d\n",
                            bb->id, phi->phi_pred_bb[k]);
                    has_errors = 1;
                }
            }

            if (phi == bb->last) break;
            ins = ins->next;
        }

        bb = bb->next;
    }

    return has_errors;
}

static int has_cycle(int u, int graph[100][100], int visited[100], int rec[100], int count) {
    visited[u] = 1;
    rec[u] = 1;
    for (int v = 0; v < count; v++) {
        if (graph[u][v]) {
            if (!visited[v] && has_cycle(v, graph, visited, rec, count)) return 1;
            else if (rec[v]) return 1;
        }
    }
    rec[u] = 0;
    return 0;
}

void ssa_destruct(CFG *cfg) {
    if (!cfg) return;

    BasicBlock *bb = cfg->blocks;
    while (bb) {
        /* Collect all phi instructions in this block */
        IRInstr *ins = bb->instrs;
        /* Skip optional leading label */
        IRInstr *scan_start = ins;
        if (scan_start && scan_start->kind == IR_LABEL)
            scan_start = scan_start->next;

        /* Collect all phi instructions in this block */
        IRInstr *phis[100];
        int phi_count = 0;
        IRInstr *scan = scan_start;
        while (scan && scan->kind == IR_PHI) {
            phis[phi_count++] = scan;
            scan = scan->next;
        }

        /* Build dependency graph for cycle detection */
        int graph[100][100] = {0};
        char *phi_results[100];
        for (int i = 0; i < phi_count; i++) {
            phi_results[i] = phis[i]->result;
        }
        for (int i = 0; i < phi_count; i++) {
            for (int k = 0; k < phis[i]->phi_arity; k++) {
                char *arg = phis[i]->phi_args[k];
                for (int j = 0; j < phi_count; j++) {
                    if (strcmp(arg, phi_results[j]) == 0) {
                        graph[i][j] = 1; /* i depends on j */
                    }
                }
            }
        }

        /* Detect cycles using DFS */
        int visited[100] = {0}, rec[100] = {0};
        int cycle = 0;
        for (int i = 0; i < phi_count; i++) {
            if (!visited[i] && has_cycle(i, graph, visited, rec, phi_count)) {
                cycle = 1;
                break;
            }
        }

        if (!cycle) {
            /* Single phase: direct assignment */
            for (int i = 0; i < phi_count; i++) {
                IRInstr *phi = phis[i];
                for (int k = 0; k < phi->phi_arity; k++) {
                    BasicBlock *pred = NULL;
                    for (int p = 0; p < bb->pred_count; p++) {
                        if (bb->preds[p]->id == phi->phi_pred_bb[k]) {
                            pred = bb->preds[p];
                            break;
                        }
                    }
                    if (!pred || !phi->phi_args[k]) continue;
                    insert_copy_before_terminator(pred, phi->result, phi->phi_args[k]);
                }
            }
        } else {
            /* Two-phase: use temps to handle cycles */
            for (int i = 0; i < phi_count; i++) {
                IRInstr *phi = phis[i];
                for (int k = 0; k < phi->phi_arity; k++) {
                    BasicBlock *pred = NULL;
                    for (int p = 0; p < bb->pred_count; p++) {
                        if (bb->preds[p]->id == phi->phi_pred_bb[k]) {
                            pred = bb->preds[p];
                            break;
                        }
                    }
                    if (!pred || !phi->phi_args[k]) continue;
                    char *temp_var = gen_phi_temp_for_var(phi->result, k);
                    if (strcmp(phi->phi_args[k], temp_var) != 0) {
                        insert_copy_before_terminator(pred, temp_var, phi->phi_args[k]);
                    }
                }
            }
            for (int i = 0; i < phi_count; i++) {
                IRInstr *phi = phis[i];
                for (int k = 0; k < phi->phi_arity; k++) {
                    BasicBlock *pred = NULL;
                    for (int p = 0; p < bb->pred_count; p++) {
                        if (bb->preds[p]->id == phi->phi_pred_bb[k]) {
                            pred = bb->preds[p];
                            break;
                        }
                    }
                    if (!pred || !phi->phi_args[k]) continue;
                    char *temp_var = gen_phi_temp_for_var(phi->result, k);
                    if (strcmp(phi->result, temp_var) != 0) {
                        insert_copy_before_terminator(pred, phi->result, temp_var);
                    }
                }
            }
        }

        /* Remove all IR_PHI nodes from bb->instrs and update bb->last safely */
        IRInstr **cur = &bb->instrs;
        IRInstr *new_last = NULL;
        while (*cur) {
            IRInstr *candidate = *cur;
            int is_last = (candidate == bb->last);
            
            /* Only remove phis that are NOT the leading label */
            if (candidate->kind == IR_PHI) {
                *cur = candidate->next;
                candidate->next = NULL;
                ir_free_instr(candidate);
                if (is_last) break; /* Block ended with a phi and we removed it */
                continue;
            }
            
            new_last = candidate;
            if (is_last) break;
            cur = &((*cur)->next);
        }
        bb->last = new_last;

        bb = bb->next;
    }
}

/* --- Loop Invariant Code Motion (LICM) --- */

static int is_loop_invariant(IRInstr *instr, int *loop_blocks, int n, CFG *cfg) {
    if (instr->kind != IR_BINOP && instr->kind != IR_UNOP && instr->kind != IR_ASSIGN && instr->kind != IR_LOAD) return 0;

    if (instr->result) {
        BasicBlock *bb = cfg->blocks;
        while (bb) {
            if (loop_blocks[bb->id]) {
                IRInstr *check = bb->instrs;
                while (check) {
                    if (check != instr && check->result && strcmp(check->result, instr->result) == 0)
                        return 0;
                    if (check == bb->last) break;
                    check = check->next;
                }
            }
            bb = bb->next;
        }
    }

    IROperand *ops[3] = {NULL};
    int num = 0;
    if (instr->kind == IR_ASSIGN) { ops[0] = &instr->src; num = 1; }
    else if (instr->kind == IR_BINOP) { ops[0] = &instr->left; ops[1] = &instr->right; num = 2; }
    else if (instr->kind == IR_UNOP) { ops[0] = &instr->unop_src; num = 1; }
    else if (instr->kind == IR_LOAD) { ops[0] = &instr->base; ops[1] = &instr->index; num = 2; }

    for (int i = 0; i < num; i++) {
        if (!ops[i]->is_const && ops[i]->name) {
            BasicBlock *bb = cfg->blocks;
            while (bb) {
                if (loop_blocks[bb->id]) {
                    IRInstr *check = bb->instrs;
                    while (check) {
                        if (check->result && strcmp(check->result, ops[i]->name) == 0) return 0;
                        if (check == bb->last) break;
                        check = check->next;
                    }
                }
                bb = bb->next;
            }
        }
    }

    if (instr->kind == IR_LOAD) {
        int has_store = 0;
        BasicBlock *bb = cfg->blocks;
        while (bb) {
            if (loop_blocks[bb->id]) {
                IRInstr *check = bb->instrs;
                while (check) {
                    if (check->kind == IR_STORE) {
                        has_store = 1;
                    }
                    if (check->kind == IR_CALL || check->kind == IR_CALL_INDIRECT) {
                        return 0;
                    }
                    if (check == bb->last) break;
                    check = check->next;
                }
            }
            bb = bb->next;
        }
        if (has_store) return 0;
    }
    return 1;
}

void optimize_loops(CFG *cfg) {
    if (!cfg) return;
    compute_dominators(cfg);

    BasicBlock *b = cfg->blocks;
    while (b) {
        for (int i = 0; i < b->succ_count; i++) {
            BasicBlock *h = b->succs[i];
            if (b->doms[h->id]) {
                int *loop_blocks = calloc(cfg->block_count, sizeof(int));
                loop_blocks[h->id] = 1;

                BasicBlock **stack = malloc(cfg->block_count * sizeof(BasicBlock*));
                int top = 0;

                if (b != h) {
                    loop_blocks[b->id] = 1;
                    stack[top++] = b;
                }

                while (top > 0) {
                    BasicBlock *m = stack[--top];
                    for (int k = 0; k < m->pred_count; k++) {
                        BasicBlock *p = m->preds[k];
                        if (!loop_blocks[p->id]) {
                            loop_blocks[p->id] = 1;
                            stack[top++] = p;
                        }
                    }
                }
                free(stack);

                BasicBlock *pre = NULL;
                for (int p = 0; p < h->pred_count; p++) {
                    if (!loop_blocks[h->preds[p]->id]) {
                        pre = h->preds[p];
                        break;
                    }
                }

                if (pre) {
                    BasicBlock *lb = cfg->blocks;
                    while (lb) {
                        if (loop_blocks[lb->id]) {
                            IRInstr *curr_ins = lb->instrs;
                            IRInstr *prev_ins = NULL;
                            while (curr_ins) {
                                IRInstr *next_ins = curr_ins->next;
                                if (curr_ins->kind != IR_GOTO && curr_ins->kind != IR_IF && is_loop_invariant(curr_ins, loop_blocks, cfg->block_count, cfg)) {
                                    if (prev_ins) prev_ins->next = next_ins;
                                    else lb->instrs = next_ins;

                                    if (curr_ins == lb->last) lb->last = prev_ins;

                                    IRInstr *pcur = pre->instrs, *pprev = NULL;
                                    while (pcur && pcur != pre->last) { pprev = pcur; pcur = pcur->next; }
                                    if (!pcur) {
                                        pre->instrs = curr_ins;
                                        curr_ins->next = NULL;
                                        pre->last = curr_ins;
                                    } else {
                                        if (pcur->kind == IR_GOTO || pcur->kind == IR_IF || pcur->kind == IR_RETURN) {
                                            if (pprev) { pprev->next = curr_ins; curr_ins->next = pcur; }
                                            else { pre->instrs = curr_ins; curr_ins->next = pcur; }
                                        } else {
                                            pcur->next = curr_ins;
                                            curr_ins->next = NULL;
                                            pre->last = curr_ins;
                                        }
                                    }
                                } else {
                                    prev_ins = curr_ins;
                                }
                                if (curr_ins == lb->last) break;
                                curr_ins = next_ins;
                            }
                        }
                        lb = lb->next;
                    }
                }
                free(loop_blocks);
            }
        }
        b = b->next;
    }
}

/* --- Loop Unrolling --- */

#define MAX_FULL_UNROLL_INSTRUCTIONS 200000

static IRInstr* clone_instr(IRInstr *src) {
    if (!src) return NULL;
    IRInstr *dup = calloc(1, sizeof(IRInstr));
    if (!dup) return NULL;

    *dup = *src;
    dup->next = NULL;

    if (src->result) dup->result = strdup(src->result);
    if (src->call_fn) dup->call_fn = strdup(src->call_fn);
    if (src->label) dup->label = strdup(src->label);

    if (src->src.name) dup->src.name = strdup(src->src.name);
    if (src->left.name) dup->left.name = strdup(src->left.name);
    if (src->right.name) dup->right.name = strdup(src->right.name);
    if (src->unop_src.name) dup->unop_src.name = strdup(src->unop_src.name);
    if (src->base.name) dup->base.name = strdup(src->base.name);
    if (src->index.name) dup->index.name = strdup(src->index.name);
    if (src->store_val.name) dup->store_val.name = strdup(src->store_val.name);
    if (src->if_left.name) dup->if_left.name = strdup(src->if_left.name);
    if (src->if_right.name) dup->if_right.name = strdup(src->if_right.name);

    return dup;
}

static int compute_natural_loop(BasicBlock *header, BasicBlock *latch, int *loop_blocks, CFG *cfg) {
    if (!header || !latch || !loop_blocks || !cfg) return 0;

    for (int i = 0; i < cfg->block_count; i++) loop_blocks[i] = 0;
    loop_blocks[header->id] = 1;

    BasicBlock **stack = malloc(sizeof(BasicBlock*) * cfg->block_count);
    if (!stack) return 0;
    int top = 0;

    if (latch != header) {
        loop_blocks[latch->id] = 1;
        stack[top++] = latch;
    }

    while (top > 0) {
        BasicBlock *m = stack[--top];
        for (int i = 0; i < m->pred_count; i++) {
            BasicBlock *p = m->preds[i];
            if (!loop_blocks[p->id]) {
                loop_blocks[p->id] = 1;
                stack[top++] = p;
            }
        }
    }

    free(stack);
    return 1;
}

static int count_block_instrs(BasicBlock *bb) {
    if (!bb || !bb->instrs) return 0;
    int n = 0;
    IRInstr *cur = bb->instrs;
    while (cur) {
        n++;
        if (cur == bb->last) break;
        cur = cur->next;
    }
    return n;
}

static int is_name_defined_in_loop(const char *name, int *loop_blocks, CFG *cfg) {
    if (!name || !loop_blocks || !cfg) return 0;
    BasicBlock *bb = cfg->blocks;
    while (bb) {
        if (loop_blocks[bb->id]) {
            IRInstr *cur = bb->instrs;
            while (cur) {
                if (cur->result && strcmp(cur->result, name) == 0) return 1;
                if (cur == bb->last) break;
                cur = cur->next;
            }
        }
        bb = bb->next;
    }
    return 0;
}

static IRRelop swap_relop(IRRelop relop) {
    switch (relop) {
        case IR_LT: return IR_GT;
        case IR_GT: return IR_LT;
        case IR_LE: return IR_GE;
        case IR_GE: return IR_LE;
        case IR_EQ: return IR_EQ;
        case IR_NE: return IR_NE;
        default: return relop;
    }
}

static int get_initial_value_from_block(BasicBlock *bb, const char *name, int *value) {
    if (!bb || !name || !value) return 0;
    IRInstr *last = NULL;
    IRInstr *cur = bb->instrs;
    while (cur) {
        if (cur->result && strcmp(cur->result, name) == 0) last = cur;
        if (cur == bb->last) break;
        cur = cur->next;
    }
    if (!last || last->kind != IR_ASSIGN || !last->src.is_const) return 0;
    *value = last->src.const_val;
    return 1;
}

static BasicBlock* find_preheader(BasicBlock *h, int *loop_blocks) {
    if (!h || !loop_blocks) return NULL;
    BasicBlock *preheader = NULL;
    for (int i = 0; i < h->pred_count; i++) {
        BasicBlock *p = h->preds[i];
        if (!loop_blocks[p->id]) {
            if (preheader) return NULL;
            preheader = p;
        }
    }
    return preheader;
}

static int find_loop_entry_exit(BasicBlock *h, int *loop_blocks, BasicBlock **body_entry_out, BasicBlock **exit_block_out) {
    if (!h || !loop_blocks || !body_entry_out || !exit_block_out) return 0;
    if (!h->last || h->last->kind != IR_IF || h->succ_count != 2) return 0;

    BasicBlock *body = NULL;
    BasicBlock *exit_block = NULL;
    for (int i = 0; i < h->succ_count; i++) {
        BasicBlock *s = h->succs[i];
        if (loop_blocks[s->id]) {
            if (body) return 0;
            body = s;
        } else {
            if (exit_block) return 0;
            exit_block = s;
        }
    }
    if (!body || !exit_block) return 0;

    *body_entry_out = body;
    *exit_block_out = exit_block;
    return 1;
}

static int has_side_exits(BasicBlock *header, int *loop_blocks, CFG *cfg, BasicBlock *exit_block) {
    if (!header || !loop_blocks || !cfg || !exit_block) return 1;
    BasicBlock *bb = cfg->blocks;
    while (bb) {
        if (loop_blocks[bb->id]) {
            for (int i = 0; i < bb->succ_count; i++) {
                BasicBlock *s = bb->succs[i];
                if (!loop_blocks[s->id]) {
                    if (bb != header || s != exit_block) {
                        return 1;
                    }
                }
            }
        }
        bb = bb->next;
    }
    return 0;
}

static int extract_delta_from_binop(IRInstr *ins, const char *ind_var, int *delta_out) {
    if (!ins || ins->kind != IR_BINOP || !ind_var || !delta_out) return 0;
    if (!ins->left.is_const && ins->left.name && strcmp(ins->left.name, ind_var) == 0 && ins->right.is_const && ins->binop == '+') {
        *delta_out = ins->right.const_val;
        return 1;
    }
    if (!ins->right.is_const && ins->right.name && strcmp(ins->right.name, ind_var) == 0 && ins->left.is_const && ins->binop == '+') {
        *delta_out = ins->left.const_val;
        return 1;
    }
    if (!ins->left.is_const && ins->left.name && strcmp(ins->left.name, ind_var) == 0 && ins->right.is_const && ins->binop == '-') {
        *delta_out = -ins->right.const_val;
        return 1;
    }
    return 0;
}

static int resolve_temp_based_delta(const char *temp_name, const char *ind_var, int *loop_blocks, CFG *cfg, int *delta_out) {
    if (!temp_name || !ind_var || !loop_blocks || !cfg || !delta_out) return 0;
    IRInstr *def = NULL;
    BasicBlock *sb = cfg->blocks;
    while (sb) {
        if (loop_blocks[sb->id]) {
            IRInstr *sc = sb->instrs;
            while (sc) {
                if (sc->result && strcmp(sc->result, temp_name) == 0) {
                    int delta = 0;
                    if (!extract_delta_from_binop(sc, ind_var, &delta)) return 0;
                    if (def) return 0;
                    def = sc;
                    *delta_out = delta;
                }
                if (sc == sb->last) break;
                sc = sc->next;
            }
        }
        sb = sb->next;
    }
    return def != NULL;
}

static int compute_loop_step(int *loop_blocks, CFG *cfg, const char *ivar, int *step_out) {
    if (!loop_blocks || !cfg || !ivar || !step_out) return 0;

    IRInstr *update = NULL;

    BasicBlock *bb = cfg->blocks;
    while (bb) {
        if (loop_blocks[bb->id]) {
            IRInstr *cur = bb->instrs;
            while (cur) {
                if (cur->result && strcmp(cur->result, ivar) == 0) {
                    int delta = 0;
                    int matched = 0;
                    if (cur->kind == IR_BINOP) {
                        matched = extract_delta_from_binop(cur, ivar, &delta);
                    } else if (cur->kind == IR_ASSIGN && !cur->src.is_const && cur->src.name) {
                        matched = resolve_temp_based_delta(cur->src.name, ivar, loop_blocks, cfg, &delta);
                    } else {
                        return 0;
                    }

                    if (!matched || delta == 0) return 0;
                    if (update) return 0;
                    update = cur;
                    *step_out = delta;
                }
                if (cur == bb->last) break;
                cur = cur->next;
            }
        }
        bb = bb->next;
    }
    return update != NULL;
}

static int resolve_operand_to_const(IROperand *op, BasicBlock *preheader, int *loop_blocks, CFG *cfg, int *value_out) {
    if (!op || !preheader || !loop_blocks || !cfg || !value_out) return 0;
    if (op->is_const) {
        *value_out = op->const_val;
        return 1;
    }
    if (!op->name) return 0;
    if (is_name_defined_in_loop(op->name, loop_blocks, cfg)) return 0;
    return get_initial_value_from_block(preheader, op->name, value_out);
}

static int extract_induction_condition(IRInstr *if_instr, BasicBlock *preheader, int *loop_blocks, CFG *cfg,
                                      const char **ivar_out, int *bound_out, IRRelop *relop_out) {
    if (!if_instr || if_instr->kind != IR_IF || !ivar_out || !bound_out || !relop_out) return 0;

    int bound = 0;
    if (if_instr->if_left.name && resolve_operand_to_const(&if_instr->if_right, preheader, loop_blocks, cfg, &bound)) {
        *ivar_out = if_instr->if_left.name;
        *bound_out = bound;
        *relop_out = if_instr->relop;
        return 1;
    }
    if (if_instr->if_right.name && resolve_operand_to_const(&if_instr->if_left, preheader, loop_blocks, cfg, &bound)) {
        *ivar_out = if_instr->if_right.name;
        *bound_out = bound;
        *relop_out = swap_relop(if_instr->relop);
        return 1;
    }
    return 0;
}

static int compute_trip_count(int init, int bound, int step, IRRelop relop, long *trip_out) {
    if (!trip_out || step == 0) return 0;

    if (step == 0) return -1;
    if (relop == IR_LT) {
        if (step > 0) {
            long diff = (long)bound - init;
            *trip_out = (diff <= 0) ? 0 : (diff + step - 1) / step;
            return 1;
        }
        return 0;
    }
    if (relop == IR_LE) {
        if (step > 0) {
            long diff = (long)bound - init;
            *trip_out = (diff < 0) ? 0 : diff / step + 1;
            return 1;
        }
        return 0;
    }
    if (relop == IR_GT) {
        if (step < 0) {
            long diff = (long)init - bound;
            long nstep = -step;
            *trip_out = (diff <= 0) ? 0 : (diff + nstep - 1) / nstep;
            return 1;
        }
        return 0;
    }
    if (relop == IR_GE) {
        if (step < 0) {
            long diff = (long)init - bound;
            long nstep = -step;
            *trip_out = (diff < 0) ? 0 : diff / nstep + 1;
            return 1;
        }
        return 0;
    }
    if (relop == IR_NE) {
        long diff = (long)bound - init;
        if (diff == 0) {
            *trip_out = 0;
            return 1;
        }
        if ((step > 0 && diff < 0) || (step < 0 && diff > 0)) return 0;
        long nstep = (step > 0) ? step : -step;
        long ndiff = (diff > 0) ? diff : -diff;
        if ((ndiff % nstep) != 0) return 0;
        *trip_out = ndiff / nstep;
        return 1;
    }
    if (relop == IR_EQ) {
        *trip_out = (init == bound) ? 1 : 0;
        return 1;
    }
    return 0;
}

static int block_label_index(const char *label, char **orig_labels, int count) {
    if (!label || !orig_labels || count <= 0) return -1;
    for (int i = 0; i < count; i++) {
        if (orig_labels[i] && strcmp(orig_labels[i], label) == 0) return i;
    }
    return -1;
}

static char** alloc_iteration_labels(int count) {
    if (count <= 0) return NULL;
    char **labels = calloc(count, sizeof(char*));
    if (!labels) return NULL;
    for (int i = 0; i < count; i++) {
        labels[i] = ir_new_label();
        if (!labels[i]) {
            for (int j = 0; j < i; j++) free(labels[j]);
            free(labels);
            return NULL;
        }
    }
    return labels;
}

static void free_iteration_labels(char **labels, int count) {
    if (!labels) return;
    for (int i = 0; i < count; i++) free(labels[i]);
    free(labels);
}

static void append_instr(IRInstr **head, IRInstr **tail, IRInstr *ins) {
    if (!head || !tail || !ins) return;
    ins->next = NULL;
    if (!*head) *head = ins;
    else (*tail)->next = ins;
    *tail = ins;
}

static void append_instr_list(IRInstr **head, IRInstr **tail, IRInstr *list_head, IRInstr *list_tail) {
    if (!head || !tail || !list_head) return;
    if (!*head) *head = list_head;
    else (*tail)->next = list_head;

    if (list_tail) {
        *tail = list_tail;
    } else {
        IRInstr *scan = list_head;
        while (scan->next) scan = scan->next;
        *tail = scan;
    }
}

typedef struct {
    char *orig;
    char *fresh;
} InternalLabelRename;

static IRInstr* clone_loop_iteration(BasicBlock **body_blocks,
                                     int body_count,
                                     char **orig_labels,
                                     char **curr_labels,
                                     const char *header_label,
                                     const char *header_target,
                                     IRInstr **tail_out) {
    if (!body_blocks || body_count <= 0 || !curr_labels || !header_label || !header_target) return NULL;

    IRInstr *head = NULL;
    IRInstr *tail = NULL;

    // Step 1: Scan for internal labels definitions that are not leading block labels
    InternalLabelRename internal_renames[512];
    int internal_rename_count = 0;

    for (int i = 0; i < body_count; i++) {
        IRInstr *cur = body_blocks[i]->instrs;
        while (cur) {
            if (cur->kind == IR_LABEL && cur->label) {
                // Check if it's already in orig_labels
                int is_orig = 0;
                for (int j = 0; j < body_count; j++) {
                    if (orig_labels[j] && strcmp(orig_labels[j], cur->label) == 0) {
                        is_orig = 1;
                        break;
                    }
                }
                // Also skip if it is the header label (already handled by header_target)
                if (!is_orig && strcmp(cur->label, header_label) != 0) {
                    // It's an internal label. Check if already seen.
                    int seen = 0;
                    for (int j = 0; j < internal_rename_count; j++) {
                        if (strcmp(internal_renames[j].orig, cur->label) == 0) {
                            seen = 1;
                            break;
                        }
                    }
                    if (!seen && internal_rename_count < 512) {
                        internal_renames[internal_rename_count].orig = cur->label;
                        internal_renames[internal_rename_count].fresh = ir_new_label();
                        internal_rename_count++;
                    }
                }
            }
            if (cur == body_blocks[i]->last) break;
            cur = cur->next;
        }
    }

    // Step 2: Clone and apply renames
    for (int i = 0; i < body_count; i++) {
        BasicBlock *bb = body_blocks[i];
        IRInstr *cur = bb->instrs;
        if (!cur) continue;

        // Handle leading label of the block
        if (cur->kind == IR_LABEL) {
            IRInstr *lbl = clone_instr(cur);
            if (!lbl) continue;
            if (lbl->label) free(lbl->label);
            lbl->label = strdup(curr_labels[i]);
            append_instr(&head, &tail, lbl);
            if (cur == bb->last) continue;
            cur = cur->next;
        } else {
            IRInstr *lbl = ir_make_label(curr_labels[i], 0);
            append_instr(&head, &tail, lbl);
        }

        while (cur) {
            IRInstr *dup = clone_instr(cur);
            if (!dup) break;

            if (dup->kind == IR_LABEL && cur->label) {
                int idx = block_label_index(cur->label, orig_labels, body_count);
                if (idx >= 0) {
                    if (dup->label) free(dup->label);
                    dup->label = strdup(curr_labels[idx]);
                } else {
                    // Check internal renames
                    for (int j = 0; j < internal_rename_count; j++) {
                        if (strcmp(internal_renames[j].orig, cur->label) == 0) {
                            if (dup->label) free(dup->label);
                            dup->label = strdup(internal_renames[j].fresh);
                            break;
                        }
                    }
                }
            }

            if ((dup->kind == IR_GOTO || dup->kind == IR_IF) && dup->label) {
                if (strcmp(dup->label, header_label) == 0) {
                    free(dup->label);
                    dup->label = strdup(header_target);
                } else {
                    int idx = block_label_index(dup->label, orig_labels, body_count);
                    if (idx >= 0) {
                        free(dup->label);
                        dup->label = strdup(curr_labels[idx]);
                    } else {
                        // Check internal renames
                        for (int j = 0; j < internal_rename_count; j++) {
                            if (strcmp(internal_renames[j].orig, dup->label) == 0) {
                                free(dup->label);
                                dup->label = strdup(internal_renames[j].fresh);
                                break;
                            }
                        }
                    }
                }
            }

            append_instr(&head, &tail, dup);
            if (cur == bb->last) break;
            cur = cur->next;
        }
    }

    // Cleanup internal renames fresh strings
    for (int i = 0; i < internal_rename_count; i++) {
        free(internal_renames[i].fresh);
    }

    if (tail_out) *tail_out = tail;
    return head;
}

static void free_block_instr_list(BasicBlock *bb) {
    if (!bb || !bb->instrs) return;
    IRInstr *cur = bb->instrs;
    while (cur) {
        IRInstr *next = cur->next;
        free_instr_single(cur);
        if (cur == bb->last) break;
        cur = next;
    }
    bb->instrs = NULL;
    bb->last = NULL;
}

static int count_loop_body_instrs(BasicBlock **body_blocks, int body_count) {
    int total = 0;
    for (int i = 0; i < body_count; i++) {
        total += count_block_instrs(body_blocks[i]);
    }
    return total;
}

static int find_block_index(BasicBlock **blocks, int count, BasicBlock *target) {
    for (int i = 0; i < count; i++) {
        if (blocks[i] == target) return i;
    }
    return -1;
}

static int build_body_block_list(CFG *cfg, int *loop_blocks, BasicBlock *header, BasicBlock **out_blocks, int max_count) {
    if (!cfg || !loop_blocks || !header || !out_blocks || max_count <= 0) return 0;
    int n = 0;
    BasicBlock *bb = cfg->blocks;
    while (bb) {
        if (loop_blocks[bb->id] && bb != header) {
            if (n >= max_count) return 0;
            out_blocks[n++] = bb;
        }
        bb = bb->next;
    }
    return n;
}


/* --- Induction Variable Elimination (IVE) --- */

typedef struct {
    char *name;
    char *base_iv;
    int multiplier;
    int offset;
    int is_basic;
    int delta;
} InductionVar;

static int iv_name_match(const char *name1, const char *name2) {
    if (!name1 || !name2) return 0;
    const char *p1 = strchr(name1, '.');
    const char *p2 = strchr(name2, '.');
    int len1 = p1 ? (int)(p1 - name1) : (int)strlen(name1);
    int len2 = p2 ? (int)(p2 - name2) : (int)strlen(name2);
    if (len1 != len2) return 0;
    return strncmp(name1, name2, len1) == 0;
}

static InductionVar* find_iv(InductionVar *ivs, int count, const char *name) {
    if (!name) return NULL;
    for (int i = 0; i < count; i++) {
        if (iv_name_match(ivs[i].name, name)) return &ivs[i];
    }
    return NULL;
}


static void __attribute__((unused)) induction_variable_elimination(CFG *cfg) {
    if (!cfg) return;
    compute_dominators(cfg);

    BasicBlock *b = cfg->blocks;
    while (b) {
        for (int j = 0; j < b->succ_count; j++) {
            BasicBlock *h = b->succs[j];
            if (!b->doms[h->id]) continue; /* only back-edges: h dominates b */

            int *loop_blocks = calloc(cfg->block_count + 1, sizeof(int));
            if (!loop_blocks) continue;

            if (!compute_natural_loop(h, b, loop_blocks, cfg)) {
                free(loop_blocks);
                continue;
            }

            BasicBlock *preheader = find_preheader(h, loop_blocks);
            if (!preheader) {
                free(loop_blocks);
                continue;
            }

            /* Step 1: Detect Basic Induction Variables (BIVs) */
            InductionVar ivs[128];
            int iv_count = 0;

            BasicBlock *lb = cfg->blocks;
            while (lb) {
                if (loop_blocks[lb->id]) {
                    IRInstr *ins = lb->instrs;
                    while (ins) {
                        int delta = 0;
                        int matched = 0;
                        char *ivar_name = NULL;

                        if (ins->kind == IR_BINOP && ins->result && ins->left.name &&
                            iv_name_match(ins->result, ins->left.name) && ins->right.is_const &&
                            (ins->binop == '+' || ins->binop == '-')) {
                            matched = 1;
                            delta = (ins->binop == '+') ? ins->right.const_val : -ins->right.const_val;
                            ivar_name = ins->result;
                        }
                        /* Pattern: tmp = i + 1; i = tmp */
                        else if (ins->kind == IR_BINOP && ins->result && ins->left.name && ins->right.is_const &&
                                 (ins->binop == '+' || ins->binop == '-')) {
                            IRInstr *nx = ins->next;
                            if (nx && nx->kind == IR_ASSIGN && !nx->src.is_const && nx->src.name &&
                                strcmp(nx->src.name, ins->result) == 0 && iv_name_match(nx->result, ins->left.name)) {
                                matched = 1;
                                delta = (ins->binop == '+') ? ins->right.const_val : -ins->right.const_val;
                                ivar_name = nx->result;
                            }
                        }

                        if (matched && iv_count < 128) {
                            ivs[iv_count].name      = strdup(ivar_name);
                            ivs[iv_count].base_iv   = strdup(ivar_name);
                            ivs[iv_count].multiplier = 1;
                            ivs[iv_count].offset    = 0;
                            ivs[iv_count].is_basic  = 1;
                            ivs[iv_count].delta     = delta;
                            iv_count++;
                        }
                        if (ins == lb->last) break;
                        ins = ins->next;
                    }
                }
                lb = lb->next;
            }

            /* Step 2: Detect Derived Induction Variables (DIVs) */
            lb = cfg->blocks;
            while (lb) {
                if (loop_blocks[lb->id]) {
                    IRInstr *ins = lb->instrs;
                    while (ins) {
                        if (ins->kind == IR_BINOP && ins->result) {
                            InductionVar *base = NULL;
                            int mult = 1, off = 0;

                            if (ins->binop == '*') {
                                base = find_iv(ivs, iv_count, ins->left.name);
                                if (base && ins->right.is_const) {
                                    mult = ins->right.const_val;
                                } else if (ins->right.name && (base = find_iv(ivs, iv_count, ins->right.name)) && ins->left.is_const) {
                                    mult = ins->left.const_val;
                                }
                                if (base && iv_count < 128) {
                                    ivs[iv_count].name       = strdup(ins->result);
                                    ivs[iv_count].base_iv    = strdup(base->base_iv);
                                    ivs[iv_count].multiplier = base->multiplier * mult;
                                    ivs[iv_count].offset     = base->offset * mult;
                                    ivs[iv_count].is_basic   = 0;
                                    ivs[iv_count].delta      = base->delta * mult;
                                    iv_count++;
                                }
                            } else if (ins->binop == '+') {
                                base = find_iv(ivs, iv_count, ins->left.name);
                                if (base && ins->right.is_const) {
                                    off = ins->right.const_val;
                                } else if (ins->right.name && (base = find_iv(ivs, iv_count, ins->right.name)) && ins->left.is_const) {
                                    off = ins->left.const_val;
                                }
                                if (base && iv_count < 128) {
                                    ivs[iv_count].name       = strdup(ins->result);
                                    ivs[iv_count].base_iv    = strdup(base->base_iv);
                                    ivs[iv_count].multiplier = base->multiplier;
                                    ivs[iv_count].offset     = base->offset + off;
                                    ivs[iv_count].is_basic   = 0;
                                    ivs[iv_count].delta      = base->delta;
                                    iv_count++;
                                }
                            }
                        }
                        if (ins == lb->last) break;
                        ins = ins->next;
                    }
                }
                lb = lb->next;
            }

            /* Step 3: Strength Reduction */
            for (int k = 0; k < iv_count; k++) {
                if (ivs[k].is_basic) continue;

                int init = 0;
                if (!get_initial_value_from_block(preheader, ivs[k].base_iv, &init)) continue;

                char *j_new = ir_new_temp();
                IROperand init_op; init_op.is_const = 1; init_op.name = NULL;
                init_op.const_val = (long)ivs[k].multiplier * init + ivs[k].offset;
                IRInstr *init_ins = ir_make_assign(strdup(j_new), init_op, h->instrs->line);

                /* Append init_ins to preheader before any branch */
                IRInstr *pcur = preheader->instrs;
                while (pcur && pcur->next) {
                    if (pcur->next->kind == IR_GOTO || pcur->next->kind == IR_IF || pcur->next->kind == IR_RETURN) break;
                    pcur = pcur->next;
                }
                if (pcur) {
                    init_ins->next = pcur->next;
                    pcur->next = init_ins;
                    if (pcur == preheader->last) preheader->last = init_ins;
                }

                /* Insert update after each BIV update inside the loop */
                lb = cfg->blocks;
                while (lb) {
                    if (loop_blocks[lb->id]) {
                        IRInstr *ins = lb->instrs;
                        while (ins) {
                            if (ins->result && iv_name_match(ins->result, ivs[k].base_iv)) {
                                IROperand j_op;     j_op.is_const = 0; j_op.name = strdup(j_new); j_op.const_val = 0;
                                IROperand delta_op; delta_op.is_const = 1; delta_op.const_val = ivs[k].delta; delta_op.name = NULL;
                                IRInstr *upd = ir_make_binop(strdup(j_new), j_op, delta_op, '+', ins->line);
                                upd->next = ins->next;
                                ins->next = upd;
                                if (ins == lb->last) lb->last = upd;
                            }
                            if (ins == lb->last) break;
                            ins = ins->next;
                        }
                    }
                    lb = lb->next;
                }

                /* Replace DIV definitions/uses with j_new */
                lb = cfg->blocks;
                while (lb) {
                    if (loop_blocks[lb->id]) {
                        IRInstr *ins = lb->instrs;
                        while (ins) {
                            if (ins->result && strcmp(ins->result, ivs[k].name) == 0) {
                                IROperand j_op; j_op.is_const = 0; j_op.name = strdup(j_new); j_op.const_val = 0;
                                convert_to_assign(ins, j_op);
                            } else {
                                IROperand *ops[5] = {NULL}; int nops = 0;
                                if (ins->kind == IR_ASSIGN)  { ops[0] = &ins->src; nops = 1; }
                                else if (ins->kind == IR_BINOP) { ops[0] = &ins->left; ops[1] = &ins->right; nops = 2; }
                                else if (ins->kind == IR_IF) { ops[0] = &ins->if_left; ops[1] = &ins->if_right; nops = 2; }
                                else if (ins->kind == IR_RETURN) { ops[0] = &ins->src; nops = 1; }
                                for (int m = 0; m < nops; m++) {
                                    if (ops[m] && !ops[m]->is_const && ops[m]->name &&
                                        strcmp(ops[m]->name, ivs[k].name) == 0) {
                                        free(ops[m]->name);
                                        ops[m]->name = strdup(j_new);
                                    }
                                }
                            }
                            if (ins == lb->last) break;
                            ins = ins->next;
                        }
                    }
                    lb = lb->next;
                }

                free(j_new);
            }

            for (int k = 0; k < iv_count; k++) {
                free(ivs[k].name);
                free(ivs[k].base_iv);
            }
            free(loop_blocks);
        }
        b = b->next;
    }
}

void unroll_loops(CFG *cfg) {

    if (!cfg) return;
    compute_dominators(cfg);

    BasicBlock *b = cfg->blocks;
    while (b) {
        for (int i = 0; i < b->succ_count; i++) {
            BasicBlock *h = b->succs[i];
            if (!b->doms[h->id]) continue;
            if (!h->last || h->last->kind != IR_IF) continue;
            if (!h->instrs || h->instrs->kind != IR_LABEL) continue;

            int *loop_blocks = calloc(cfg->block_count, sizeof(int));
            if (!compute_natural_loop(h, b, loop_blocks, cfg)) {
                free(loop_blocks);
                continue;
            }

            BasicBlock *preheader = find_preheader(h, loop_blocks);
            if (!preheader) {
                free(loop_blocks);
                continue;
            }

            BasicBlock *body_entry = NULL;
            BasicBlock *exit_block = NULL;
            if (!find_loop_entry_exit(h, loop_blocks, &body_entry, &exit_block)) {
                free(loop_blocks);
                continue;
            }
            if (has_side_exits(h, loop_blocks, cfg, exit_block)) {
                free(loop_blocks);
                continue;
            }

            if (!exit_block->instrs || exit_block->instrs->kind != IR_LABEL || !exit_block->instrs->label) {
                free(loop_blocks);
                continue;
            }

            const char *ivar = NULL;
            IRInstr *if_instr = h->last;
            int bound = 0;
            IRRelop relop;
            if (!extract_induction_condition(if_instr, preheader, loop_blocks, cfg, &ivar, &bound, &relop)) {
                free(loop_blocks);
                continue;
            }

            BasicBlock *if_taken = find_bb_by_label(cfg->blocks, if_instr->label);
            if (if_taken && if_taken == exit_block) {
                relop = negate_relop(relop);
            }

            int init = 0;
            if (!get_initial_value_from_block(preheader, ivar, &init)) {
                free(loop_blocks);
                continue;
            }

            int step = 0;
            if (!compute_loop_step(loop_blocks, cfg, ivar, &step)) {
                free(loop_blocks);
                continue;
            }

            long trip_count = 0;
            if (!compute_trip_count(init, bound, step, relop, &trip_count)) {
                free(loop_blocks);
                continue;
            }

            BasicBlock *body_blocks[256];
            int body_count = build_body_block_list(cfg, loop_blocks, h, body_blocks, 256);
            if (body_count <= 0) {
                free(loop_blocks);
                continue;
            }

            int entry_idx = find_block_index(body_blocks, body_count, body_entry);
            if (entry_idx < 0) {
                free(loop_blocks);
                continue;
            }

            int body_instrs = count_loop_body_instrs(body_blocks, body_count);
            if (trip_count > 0 && body_instrs > 0) {
                if (trip_count > (MAX_FULL_UNROLL_INSTRUCTIONS / body_instrs)) {
                    free(loop_blocks);
                    continue;
                }
            }

            const char *header_label = h->instrs->label;
            const char *exit_label = exit_block->instrs->label;

            char *orig_labels[256];
            for (int j = 0; j < body_count; j++) {
                IRInstr *ins = body_blocks[j]->instrs;
                orig_labels[j] = (ins && ins->kind == IR_LABEL && ins->label) ? ins->label : NULL;
            }

            IRInstr *new_head = ir_make_label((char *)header_label, h->instrs->line);
            IRInstr *new_tail = new_head;
            if (!new_head) {
                free(loop_blocks);
                continue;
            }

            if (trip_count == 0) {
                IRInstr *to_exit = ir_make_goto((char *)exit_label, if_instr->line);
                append_instr(&new_head, &new_tail, to_exit);
                free_block_instr_list(h);
                h->instrs = new_head;
                h->last = new_tail;
                free(loop_blocks);
                continue;
            }

            char **curr_labels = alloc_iteration_labels(body_count);
            if (!curr_labels) {
                free_block_instr_list(h);
                free(loop_blocks);
                continue;
            }

            IRInstr *jump_to_first = ir_make_goto(curr_labels[entry_idx], if_instr->line);
            append_instr(&new_head, &new_tail, jump_to_first);

            int ok = 1;
            for (long iter = 0; iter < trip_count; iter++) {
                char **next_labels = NULL;
                const char *next_target = exit_label;

                if (iter + 1 < trip_count) {
                    next_labels = alloc_iteration_labels(body_count);
                    if (!next_labels) {
                        ok = 0;
                        break;
                    }
                    next_target = next_labels[entry_idx];
                }

                IRInstr *iter_tail = NULL;
                IRInstr *iter_head = clone_loop_iteration(body_blocks, body_count, orig_labels, curr_labels,
                                                         header_label, next_target, &iter_tail);
                if (!iter_head || !iter_tail) {
                    ok = 0;
                    if (next_labels) free_iteration_labels(next_labels, body_count);
                    break;
                }

                append_instr_list(&new_head, &new_tail, iter_head, iter_tail);

                free_iteration_labels(curr_labels, body_count);
                curr_labels = next_labels;
            }
            if (curr_labels) free_iteration_labels(curr_labels, body_count);

            if (!ok) {
                free_block_instr_list(h);
                h->instrs = new_head;
                h->last = new_tail;
                free(loop_blocks);
                continue;
            }

            free_block_instr_list(h);
            h->instrs = new_head;
            h->last = new_tail;
            free(loop_blocks);
        }
        b = b->next;
    }
}

static void merge_trivial_blocks(CFG *cfg) {
    if (!cfg || !cfg->blocks) return;
    int changed = 1;
    while (changed) {
        changed = 0;
        BasicBlock *bb = cfg->blocks;
        while (bb) {
            if (bb->succ_count == 1) {
                BasicBlock *succ = bb->succs[0];
                if (succ->pred_count == 1 && succ != bb && succ != cfg->entry && succ == bb->next) {
                    if (bb->last && bb->last->kind == IR_GOTO) {
                        IRInstr *p = bb->instrs, *prev = NULL;
                        while(p && p != bb->last) { prev = p; p = p->next; }
                        if (prev) prev->next = NULL;
                        else bb->instrs = NULL;
                        free_instr_single(bb->last);
                        bb->last = prev;
                    }
                    IRInstr *to_add = succ->instrs;
                    if (to_add && to_add->kind == IR_LABEL) {
                        IRInstr *lbl = to_add;
                        to_add = to_add->next;
                        free_instr_single(lbl);
                    }
                    if (to_add) {
                        if (bb->last) bb->last->next = to_add;
                        else bb->instrs = to_add;
                        bb->last = succ->last;
                    }
                    free(bb->succs);
                    bb->succ_count = succ->succ_count;
                    bb->succs = malloc(sizeof(BasicBlock*) * bb->succ_count);
                    for (int i = 0; i < bb->succ_count; i++) {
                        bb->succs[i] = succ->succs[i];
                        BasicBlock *child = bb->succs[i];
                        for (int j = 0; j < child->pred_count; j++) {
                            if (child->preds[j] == succ) child->preds[j] = bb;
                        }
                    }
                    succ->instrs = NULL;
                    succ->last = NULL;
                    succ->pred_count = 0;
                    succ->succ_count = 0;
                    bb->next = succ->next;
                    free(succ);
                    changed = 1;
                }
            }
            bb = bb->next;
        }
    }
}

/* --- Tail Call Optimization (TCO) Detection --- */

static int is_tail_position(IRInstr *instr) {
    IRInstr *next = instr->next;
    while (next) {
        if (next->kind != IR_LABEL) return 0;
        next = next->next;
    }
    return 1;
}

static void detect_tail_calls(IRFunc *f) {
    if (!f || !f->instrs) return;

    IRInstr *curr = f->instrs;
    while (curr) {
        if (curr->kind == IR_CALL || curr->kind == IR_CALL_INDIRECT) {
            int is_tail = 0;
            IRInstr *next = curr->next;

            if (next && next->kind == IR_RETURN) {
                if (!curr->result && !next->src.name && !next->src.is_const) is_tail = 1;
                else if (curr->result && next->src.name && strcmp(curr->result, next->src.name) == 0) is_tail = 1;
            } else if (!next || is_tail_position(curr)) {
                if (!curr->result) is_tail = 1;
            }

            if (is_tail && curr->call_fn && f->name && strcmp(curr->call_fn, f->name) == 0) {
                curr->is_tail_call = 1;
            }
        }
        curr = curr->next;
    }
}

void optimize_program(IRProgram *prog, OptLevel level, CompilerMetrics *metrics) {
    if (!prog) return;

    IRFunc *f = prog->funcs;
    while (f) {
        if (level > OPT_O0)
            f->instrs = simplify_control_flow(f->instrs);

        CFG *cfg = build_cfg(f);
        if (cfg) {
            char cfg_path[128];
            snprintf(cfg_path, sizeof(cfg_path), "%s_cfg.json", f->name);
            export_cfg_to_json(cfg, cfg_path);

            if (level == OPT_O0) {
                free_cfg(cfg);
                f = f->next;
                continue;
            }

            BasicBlock *bb = cfg->blocks;
            while (bb) {
                optimize_bb(bb);
                bb = bb->next;
            }

            mark_reachable_and_cleanup(cfg);
            eliminate_dead_code(cfg, metrics);

            merge_trivial_blocks(cfg);
            mark_reachable_and_cleanup(cfg);

            if (level >= OPT_O2) {
                /* --- SSA round-trip (Phase 1-3) ---
                 * Dominators must be computed before constructing SSA because
                 * both dominance frontiers and dominator-tree renaming rely on them.
                 * After all SSA-based passes (currently none), destruct back to
                 * conventional 3-address IR before the loop passes run.
                 */
                compute_dominators(cfg);
                // ssa_construct(cfg);
                /* <-- Future SSA-based passes go here (SCCP, GVN, SSA-DCE, ...) */
                // ssa_destruct(cfg);

                /* Temporarily disabled: current IVE can miscompile loops with branches
                 * by over-aggressively rewriting derived values.
                 * Re-enable only after dominance/use-safety checks are strengthened.
                 */
                // induction_variable_elimination(cfg);
                optimize_loops(cfg);
                unroll_loops(cfg);

            }

            f->instrs = flatten_cfg(cfg);
            free_cfg(cfg);
            
            f->instrs = simplify_control_flow(f->instrs);
            
            cfg = build_cfg(f);
            if (cfg) {
                mark_reachable_and_cleanup(cfg);
                f->instrs = flatten_cfg(cfg);
                free_cfg(cfg);
            }
            
            f->instrs = simplify_control_flow(f->instrs);
            detect_tail_calls(f);
        }
        f = f->next;
    }
}
