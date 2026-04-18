#ifndef IR_OPT_H
#define IR_OPT_H

#include "ir.h"

struct CompilerMetrics;

/* Optimization level */
typedef enum {
    OPT_O0,
    OPT_O1,
    OPT_O2
} OptLevel;

/* Basic Block structure */
typedef struct BasicBlock {
    int id;
    IRInstr *instrs;    /* Pointer to first instruction in block */
    IRInstr *last;      /* Pointer to last instruction in block */
    
    /* Control Flow */
    struct BasicBlock **preds;
    int pred_count;
    struct BasicBlock **succs;
    int succ_count;

    /* Liveness analysis */
    char **live_in;
    int live_in_count;
    char **live_out;
    int live_out_count;

    /* Gen/Kill (Use/Def) for liveness */
    char **use;
    int use_count;
    char **def;
    int def_count;
    
    /* Dominators */
    int *doms; /* bitset of block IDs: doms[i]=1 means block i dominates this block */

    /* Dominance Frontier (SSA construction) */
    int *df;   /* bitset of block IDs: df[i]=1 means block i is in this block's DF */

    struct BasicBlock *next; /* For linear list of blocks in function */
} BasicBlock;

/* Control Flow Graph for a function */
typedef struct CFG {
    char *func_name;
    BasicBlock *entry;
    BasicBlock *blocks;
    int block_count;
} CFG;

/* Main entry point for IR optimizations (metrics may be NULL; O0 skips all IR opts) */
void optimize_program(IRProgram *prog, OptLevel level, struct CompilerMetrics *metrics);

/* CFG Lifecycle */
CFG* build_cfg(IRFunc *f);
void free_cfg(CFG *cfg);
IRInstr* flatten_cfg(CFG *cfg);
void export_cfg_to_json(CFG *cfg, const char *path);

/* Liveness analysis (also used by register allocator) */
void compute_liveness(CFG *cfg);

/* Dominator analysis */
void compute_dominators(CFG *cfg);
void compute_dominance_frontiers(CFG *cfg); /* requires compute_dominators first */
int  idom_of(CFG *cfg, BasicBlock *b);      /* returns id of immediate dominator, -1 for entry */

// /* Dead code / unreachable block elimination */
// void eliminate_dead_code(CFG *cfg);

void eliminate_unreachable_blocks(CFG *cfg);

/* SSA Construction & Deconstruction (optimizer-internal; always paired) */
void ssa_construct(CFG *cfg); /* convert to SSA: insert phis + rename variables */
void ssa_destruct(CFG *cfg);  /* out-of-SSA: replace phis with copy instructions */

#endif /* IR_OPT_H */
