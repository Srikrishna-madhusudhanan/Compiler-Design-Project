#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ir_sched.h"

/* -------------------------------------------------------------------------
 * Instruction Scheduler internal structures
 * ------------------------------------------------------------------------- */

typedef struct SchedNode SchedNode;
typedef struct SchedEdge SchedEdge;

struct SchedEdge {
    SchedNode *target;
    SchedEdge *next;
};

struct SchedNode {
    IRInstr *instr;
    int index;
    int latency;
    
    /* Flags for dependencies */
    int is_load;
    int is_store;
    int is_call;
    int is_barrier; /* control flow, labels, etc. */
    
    /* Edges */
    SchedEdge *succs; /* nodes that depend on this one */
    SchedEdge *preds; /* nodes that this one depends on */
    int num_preds;
    int num_preds_unscheduled;
    
    /* Priority */
    int est_completion_time;
};

/* -------------------------------------------------------------------------
 * Dependency Tracking
 * ------------------------------------------------------------------------- */

/* We track variable defs/uses to add RAW, WAR, WAW edges */
typedef struct VarState {
    const char *name;
    SchedNode *last_def;
    
    SchedNode **last_uses;
    int num_uses;
    int use_cap;
} VarState;

typedef struct {
    VarState *vars;
    int count;
    int cap;
    
    /* Memory dependence tracking */
    SchedNode *last_store;
    SchedNode *last_call;
    SchedNode *last_param;
    
    /* Loads since last store/call */
    SchedNode **recent_loads;
    int num_loads;
    int load_cap;
} DepTracker;

static void init_tracker(DepTracker *t) {
    memset(t, 0, sizeof(*t));
}

static void free_tracker(DepTracker *t) {
    for (int i = 0; i < t->count; i++) {
        if (t->vars[i].last_uses) free(t->vars[i].last_uses);
    }
    if (t->vars) free(t->vars);
    if (t->recent_loads) free(t->recent_loads);
}

static VarState *get_var(DepTracker *t, const char *name) {
    if (!name) return NULL;
    for (int i = 0; i < t->count; i++) {
        if (strcmp(t->vars[i].name, name) == 0) return &t->vars[i];
    }
    if (t->count == t->cap) {
        t->cap = t->cap ? t->cap * 2 : 16;
        t->vars = realloc(t->vars, sizeof(VarState) * t->cap);
    }
    VarState *v = &t->vars[t->count++];
    memset(v, 0, sizeof(*v));
    v->name = name;
    return v;
}

static void add_edge(SchedNode *pred, SchedNode *succ) {
    if (pred == succ) return;
    
    /* Avoid duplicates */
    for (SchedEdge *e = pred->succs; e; e = e->next) {
        if (e->target == succ) return;
    }
    
    SchedEdge *e = malloc(sizeof(SchedEdge));
    e->target = succ;
    e->next = pred->succs;
    pred->succs = e;
    
    SchedEdge *pe = malloc(sizeof(SchedEdge));
    pe->target = pred;
    pe->next = succ->preds;
    succ->preds = pe;
    
    succ->num_preds++;
}

static void record_use(DepTracker *t, SchedNode *n, const char *name) {
    if (!name) return;
    VarState *v = get_var(t, name);
    
    /* RAW: this node reads, so it depends on last def */
    if (v->last_def) add_edge(v->last_def, n);
    
    /* Record this use */
    if (v->num_uses == v->use_cap) {
        v->use_cap = v->use_cap ? v->use_cap * 2 : 4;
        v->last_uses = realloc(v->last_uses, sizeof(SchedNode*) * v->use_cap);
    }
    v->last_uses[v->num_uses++] = n;
}

static void record_def(DepTracker *t, SchedNode *n, const char *name) {
    if (!name) return;
    VarState *v = get_var(t, name);
    
    /* WAW: this node writes, so it depends on last def */
    if (v->last_def) add_edge(v->last_def, n);
    
    /* WAR: this node writes, so it depends on all prior reads */
    for (int i = 0; i < v->num_uses; i++) {
        add_edge(v->last_uses[i], n);
    }
    /* Clear uses since we have a new def */
    v->num_uses = 0;
    
    /* Update last def */
    v->last_def = n;
}

/* -------------------------------------------------------------------------
 * Basic Block DAG Construction
 * ------------------------------------------------------------------------- */

static int is_barrier(IRInstr *instr) {
    if (!instr) return 0;
    switch (instr->kind) {
        case IR_LABEL:
        case IR_GOTO:
        case IR_IF:
        case IR_RETURN:
        case IR_TRY_BEGIN:
        case IR_TRY_END:
        case IR_THROW:
            return 1;
        default: return 0;
    }
}

static void build_dag(SchedNode *nodes, int count) {
    DepTracker tracker;
    init_tracker(&tracker);
    
    SchedNode *last_barrier = NULL;
    
    for (int i = 0; i < count; i++) {
        SchedNode *n = &nodes[i];
        IRInstr *inst = n->instr;
        
        /* If there's a barrier before us, we must depend on it */
        if (last_barrier) add_edge(last_barrier, n);
        
        if (n->is_barrier) {
            /* We depend on EVERYTHING before us */
            for (int j = 0; j < i; j++) add_edge(&nodes[j], n);
            last_barrier = n;
            continue;
        }
        
        /* Memory dependencies */
        if (n->is_call) {
            /* Call depends on prior calls, stores, and loads */
            if (tracker.last_call) add_edge(tracker.last_call, n);
            if (tracker.last_store) add_edge(tracker.last_store, n);
            for (int j = 0; j < tracker.num_loads; j++) 
                add_edge(tracker.recent_loads[j], n);
            tracker.last_call = n;
            tracker.num_loads = 0; /* Call acts as a barrier, reset loads */
        }
        else if (n->is_store) {
            if (tracker.last_call) add_edge(tracker.last_call, n);
            if (tracker.last_store) add_edge(tracker.last_store, n);
            for (int j = 0; j < tracker.num_loads; j++) 
                add_edge(tracker.recent_loads[j], n);
            tracker.last_store = n;
            tracker.num_loads = 0; 
        }
        else if (n->is_load) {
            if (tracker.last_call) add_edge(tracker.last_call, n);
            if (tracker.last_store) add_edge(tracker.last_store, n);
            
            if (tracker.num_loads == tracker.load_cap) {
                tracker.load_cap = tracker.load_cap ? tracker.load_cap * 2 : 4;
                tracker.recent_loads = realloc(tracker.recent_loads, sizeof(SchedNode*) * tracker.load_cap);
            }
            tracker.recent_loads[tracker.num_loads++] = n;
        }
        
        /* Register/Variable dependencies */
        IROperand *uses[6] = {0};
        int n_use = 0;
        char *def = NULL;
        
        switch (inst->kind) {
            case IR_ASSIGN: 
                uses[n_use++] = &inst->src; 
                def = inst->result; 
                break;
            case IR_BINOP:  
                uses[n_use++] = &inst->left; 
                uses[n_use++] = &inst->right; 
                def = inst->result; 
                break;
            case IR_UNOP:   
                uses[n_use++] = &inst->unop_src; 
                def = inst->result; 
                break;
            case IR_PARAM:  
                uses[n_use++] = &inst->src; 
                break;
            case IR_CALL:   
                def = inst->result; 
                break;
            case IR_CALL_INDIRECT:
                uses[n_use++] = &inst->base;
                def = inst->result; 
                break;
            case IR_LOAD:   
                uses[n_use++] = &inst->base; 
                uses[n_use++] = &inst->index; 
                def = inst->result; 
                break;
            case IR_STORE:  
                uses[n_use++] = &inst->base; 
                uses[n_use++] = &inst->index; 
                uses[n_use++] = &inst->store_val; 
                break;
            case IR_TRY_BEGIN:
                /* try_begin is a call and a control flow barrier */
                break;
            case IR_THROW:
                uses[n_use++] = &inst->src;
                /* throw is a call */
                break;
            default: break;
        }
        
        for (int u = 0; u < n_use; u++) {
            if (!uses[u]->is_const) record_use(&tracker, n, uses[u]->name);
        }
        if (def) record_def(&tracker, n, def);

        /* Handle Param/Call ordering */
        if (inst->kind == IR_PARAM) {
            if (tracker.last_param) add_edge(tracker.last_param, n);
            if (tracker.last_call) add_edge(tracker.last_call, n);
            tracker.last_param = n;
        } else if (inst->kind == IR_CALL || inst->kind == IR_CALL_INDIRECT) {
            if (tracker.last_param) add_edge(tracker.last_param, n);
            tracker.last_call = n;
            tracker.last_param = NULL;
        }
    }
    
    free_tracker(&tracker);
}

/* -------------------------------------------------------------------------
 * Critical Path Calculation
 * ------------------------------------------------------------------------- */

static void compute_priorities(SchedNode *nodes, int count) {
    /* Initialize latencies */
    for (int i = 0; i < count; i++) {
        nodes[i].latency = nodes[i].is_load ? 3 : 1; /* Assign higher latency to loads */
        nodes[i].est_completion_time = -1;
    }
    
    /* Calculate recursive longest path (simple memoization) */
    int changed = 1;
    while (changed) {
        changed = 0;
        for (int i = count - 1; i >= 0; i--) {
            int max_succ = 0;
            for (SchedEdge *e = nodes[i].succs; e; e = e->next) {
                if (e->target->est_completion_time > max_succ) {
                    max_succ = e->target->est_completion_time;
                }
            }
            int new_time = max_succ + nodes[i].latency;
            if (new_time > nodes[i].est_completion_time) {
                nodes[i].est_completion_time = new_time;
                changed = 1;
            }
        }
    }
}

/* -------------------------------------------------------------------------
 * List Scheduling algorithm
 * ------------------------------------------------------------------------- */

static IRInstr *schedule_block(IRInstr *first, IRInstr *last, int count) {
    if (count <= 1) return first;
    
    SchedNode *nodes = calloc(count, sizeof(SchedNode));
    
    IRInstr *curr = first;
    for (int i = 0; i < count; i++) {
        nodes[i].instr = curr;
        nodes[i].index = i;
        nodes[i].is_barrier = is_barrier(curr);
        nodes[i].is_load = (curr->kind == IR_LOAD);
        nodes[i].is_store = (curr->kind == IR_STORE);
        nodes[i].is_call = (curr->kind == IR_CALL || curr->kind == IR_CALL_INDIRECT || curr->kind == IR_TRY_BEGIN || curr->kind == IR_THROW);
        curr = curr->next;
    }
    
    build_dag(nodes, count);
    compute_priorities(nodes, count);
    
    /* Initialize ready counts */
    for (int i = 0; i < count; i++) {
        nodes[i].num_preds_unscheduled = nodes[i].num_preds;
    }
    
    /* Scheduling state */
    IRInstr *new_head = NULL;
    IRInstr *new_tail = NULL;
    int scheduled = 0;
    
    /* Simple list scheduling, no cycle counting needed, just order instructions */
    while (scheduled < count) {
        /* Find best ready node */
        SchedNode *best = NULL;
        
        for (int i = 0; i < count; i++) {
            if (nodes[i].num_preds_unscheduled == 0) {
                if (!best) {
                    best = &nodes[i];
                } else if (nodes[i].est_completion_time > best->est_completion_time) {
                    best = &nodes[i]; /* prioritize critical path */
                } else if (nodes[i].est_completion_time == best->est_completion_time) {
                    if (nodes[i].index < best->index) {
                        best = &nodes[i]; /* tie-breaker: original order */
                    }
                }
            }
        }
        
        if (!best) {
            /* Should never happen for DAG, implies cycle */
            fprintf(stderr, "CYCLE detected during IR list scheduling!\n");
            break;
        }
        
        /* Schedule it */
        best->num_preds_unscheduled = -1; /* mark completed */
        scheduled++;
        
        /* Decrement predecessors count of successors */
        for (SchedEdge *e = best->succs; e; e = e->next) {
            e->target->num_preds_unscheduled--;
        }
        
        /* Append to new list */
        if (!new_head) {
            new_head = best->instr;
            new_tail = best->instr;
        } else {
            new_tail->next = best->instr;
            new_tail = best->instr;
        }
    }
    
    /* Terminate list */
    if (new_tail) new_tail->next = NULL;
    
    /* Free structures */
    for (int i = 0; i < count; i++) {
        SchedEdge *e = nodes[i].succs;
        while (e) {
            SchedEdge *ne = e->next;
            free(e);
            e = ne;
        }
        e = nodes[i].preds;
        while (e) {
            SchedEdge *ne = e->next;
            free(e);
            e = ne;
        }
    }
    free(nodes);
    
    return new_head;
}

/* -------------------------------------------------------------------------
 * Public Interface
 * ------------------------------------------------------------------------- */

void ir_schedule_function(IRFunc *f) {
    if (!f || !f->instrs) return;
    
    IRInstr *curr = f->instrs;
    IRInstr *first_of_block = curr;
    IRInstr *prev_of_block = NULL;
    IRInstr *new_func_head = NULL;
    int bcount = 0;
    
    while (curr) {
        bcount++;
        int ends_block = is_barrier(curr) || (curr->next && curr->next->kind == IR_LABEL);
        
        if (ends_block || !curr->next) {
            IRInstr *next_block = curr->next;
            curr->next = NULL;
            
            IRInstr *s_head = schedule_block(first_of_block, curr, bcount);
            IRInstr *s_tail = s_head;
            while (s_tail && s_tail->next) s_tail = s_tail->next;
            
            if (prev_of_block) prev_of_block->next = s_head;
            else new_func_head = s_head;
            
            s_tail->next = next_block;
            prev_of_block = s_tail;
            
            first_of_block = next_block;
            curr = next_block;
            bcount = 0;
        } else {
            curr = curr->next;
        }
    }
    f->instrs = new_func_head;
}

void ir_schedule_export_json(IRFunc *f, const char *path) {
    if (!f || !path) return;
    FILE *fp = fopen(path, "w");
    if (!fp) return;

    fprintf(fp, "{\n");
    fprintf(fp, "  \"func_name\": \"%s\",\n", f->name);
    fprintf(fp, "  \"blocks\": [\n");

    /*
     * We group instructions into semantic basic blocks:
     *   - A block starts right after a label (or at function entry)
     *   - A block ends at a control-flow barrier (IF, GOTO, RETURN, CALL etc.)
     *   - Labels themselves are separators, not emitted as nodes
     *
     * This ensures each block has multiple real instructions, not just 1.
     */

    /* First pass: collect all instruction pointers (skip labels) */
    typedef struct { IRInstr **instrs; int count; int cap; } Block;
    Block blocks[256];
    int block_count = 0;

    blocks[0].instrs = NULL;
    blocks[0].count  = 0;
    blocks[0].cap    = 0;
    block_count = 1;

    for (IRInstr *cur = f->instrs; cur; cur = cur->next) {
        if (cur->kind == IR_LABEL) {
            /* Start a new block after this label */
            if (blocks[block_count - 1].count > 0 && block_count < 255) {
                blocks[block_count].instrs = NULL;
                blocks[block_count].count  = 0;
                blocks[block_count].cap    = 0;
                block_count++;
            }
            continue; /* don't add labels as nodes */
        }

        Block *cb = &blocks[block_count - 1];
        if (cb->count >= cb->cap) {
            cb->cap = cb->cap ? cb->cap * 2 : 8;
            cb->instrs = realloc(cb->instrs, cb->cap * sizeof(IRInstr *));
        }
        cb->instrs[cb->count++] = cur;

        /* End this block at control-flow but continue to next */
        if (is_barrier(cur)) {
            if (block_count < 255) {
                blocks[block_count].instrs = NULL;
                blocks[block_count].count  = 0;
                blocks[block_count].cap    = 0;
                block_count++;
            }
        }
    }

    /* Second pass: emit each block */
    int first_block = 1;
    for (int b = 0; b < block_count; b++) {
        Block *cb = &blocks[b];
        if (cb->count == 0) { free(cb->instrs); continue; }

        int count = cb->count;

        /* Save original next pointers */
        IRInstr **saved_nexts = malloc(count * sizeof(IRInstr *));
        for (int i = 0; i < count; i++) {
            saved_nexts[i] = cb->instrs[i]->next;
        }

        /* Build a temporary instruction chain for DAG building */
        for (int i = 0; i < count - 1; i++)
            cb->instrs[i]->next = cb->instrs[i + 1];
        cb->instrs[count - 1]->next = NULL;

        SchedNode *nodes = calloc(count, sizeof(SchedNode));
        for (int i = 0; i < count; i++) {
            nodes[i].instr      = cb->instrs[i];
            nodes[i].index      = i;
            nodes[i].is_barrier = is_barrier(cb->instrs[i]);
            nodes[i].is_load    = (cb->instrs[i]->kind == IR_LOAD);
            nodes[i].is_store   = (cb->instrs[i]->kind == IR_STORE);
            nodes[i].is_call    = (cb->instrs[i]->kind == IR_CALL || cb->instrs[i]->kind == IR_CALL_INDIRECT);
        }
        build_dag(nodes, count);
        compute_priorities(nodes, count);

        if (!first_block) fprintf(fp, ",\n");
        first_block = 0;

        fprintf(fp, "    {\n");
        fprintf(fp, "      \"id\": %d,\n", b + 1);

        /* Nodes */
        fprintf(fp, "      \"nodes\": [\n");
        for (int i = 0; i < count; i++) {
            char buf[256];
            ir_snprint_instr(buf, sizeof(buf), nodes[i].instr);
            /* Escape backslashes and quotes for JSON */
            char escaped[512]; int ei = 0;
            for (int ci = 0; buf[ci] && ei < 510; ci++) {
                if (buf[ci] == '"' || buf[ci] == '\\') escaped[ei++] = '\\';
                escaped[ei++] = buf[ci];
            }
            escaped[ei] = '\0';
            fprintf(fp, "        {\"id\": %d, \"label\": \"%s\", \"priority\": %d}%s\n",
                    i, escaped, nodes[i].est_completion_time, (i == count - 1) ? "" : ",");
        }
        fprintf(fp, "      ],\n");

        /* Edges */
        fprintf(fp, "      \"edges\": [\n");
        int first_edge = 1;
        for (int i = 0; i < count; i++) {
            for (SchedEdge *e = nodes[i].succs; e; e = e->next) {
                if (!first_edge) fprintf(fp, ",\n");
                fprintf(fp, "        {\"from\": %d, \"to\": %d}", i, e->target->index);
                first_edge = 0;
            }
        }
        fprintf(fp, "\n      ]\n");
        fprintf(fp, "    }");

        /* Cleanup edges */
        for (int i = 0; i < count; i++) {
            SchedEdge *e = nodes[i].succs;
            while (e) { SchedEdge *ne = e->next; free(e); e = ne; }
            e = nodes[i].preds;
            while (e) { SchedEdge *ne = e->next; free(e); e = ne; }
        }
        free(nodes);
        
        /* Restore original next pointers */
        for (int i = 0; i < count; i++) {
            cb->instrs[i]->next = saved_nexts[i];
        }
        free(saved_nexts);
        free(cb->instrs);
    }

    fprintf(fp, "\n  ]\n");
    fprintf(fp, "}\n");
    fclose(fp);
}

