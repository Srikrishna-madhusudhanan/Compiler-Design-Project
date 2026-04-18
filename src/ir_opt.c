#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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
            if (curr->kind == IR_GOTO || curr->kind == IR_IF || curr->kind == IR_RETURN) {
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
        } else if (last->kind == IR_IF) {
            BasicBlock *target = find_bb_by_label(head, last->label);
            if (target) add_succ(bb, target);
            if (bb->next) add_succ(bb, bb->next);
        } else if (last->kind != IR_RETURN) {
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
        if (bb->doms) free(bb->doms);
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

            if (curr->kind == IR_GOTO || curr->kind == IR_RETURN) {
                while (curr->next && curr->next->kind != IR_LABEL) {
                    IRInstr *to_del = curr->next;
                    curr->next = to_del->next;
                    to_del->next = NULL;
                    free_instr_single(to_del);
                    changed = 1;
                }
            }

            if (curr->kind == IR_IF && curr->next && curr->next->kind == IR_GOTO &&
                curr->next->next && curr->next->next->kind == IR_LABEL &&
                strcmp(curr->label, curr->next->next->label) == 0) {

                char *target_L2 = curr->next->label;
                IRRelop neg_rel = negate_relop(curr->relop);
                IRInstr *goto_instr = curr->next;
                IRInstr *label_L1 = goto_instr->next;

                free(curr->label);
                curr->label = target_L2 ? strdup(target_L2) : NULL;
                curr->relop = neg_rel;
                curr->next = label_L1;

                goto_instr->label = NULL;
                goto_instr->next = NULL;
                free_instr_single(goto_instr);
                changed = 1;
                continue;
            }

            if (curr->kind == IR_GOTO && curr->next && curr->next->kind == IR_LABEL &&
                strcmp(curr->label, curr->next->label) == 0) {

                *curr_ptr = curr->next;
                curr->next = NULL;
                free_instr_single(curr);
                changed = 1;
                continue;
            }

            if (curr->kind == IR_GOTO || curr->kind == IR_IF) {
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
    else if (instr->kind == IR_RETURN) { ops[0] = &instr->src; num_ops = 1; }
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
        else if (curr->kind == IR_RETURN) { ops[0] = &curr->src; num_ops = 1; }
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

    for (int i = 0; i < body_count; i++) {
        BasicBlock *bb = body_blocks[i];
        IRInstr *cur = bb->instrs;
        if (!cur) continue;

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
                    }
                }
            }

            append_instr(&head, &tail, dup);
            if (cur == bb->last) break;
            cur = cur->next;
        }
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