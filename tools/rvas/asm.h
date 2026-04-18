#ifndef RVAS_ASM_H
#define RVAS_ASM_H

#include "rvas.h"
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

typedef struct {
    uint8_t *data;
    size_t len;
    size_t cap;
} RvBuf;

typedef struct {
    char *name;
    int sec; /* 0=undef, 1=text, 2=rodata, 3=data */
    uint64_t value;
    bool defined;
    bool global;
} RvSym;

typedef enum {
    RV_FIX_BRANCH,
    RV_FIX_JAL,
    RV_FIX_HI20,
    RV_FIX_LO12,
    RV_FIX_CALL_PLT,
    RV_FIX_ABS64,
    RV_FIX_ABS32,
} RvFixKind;


typedef struct {
    RvFixKind kind;
    int sec;
    size_t off;
    char *sym;
    bool patched;
} RvFixup;

typedef struct {
    RvBuf text, rodata, data;
    RvSym *syms;
    size_t sym_count;
    RvFixup *fixups;
    size_t fix_count;
    unsigned pcrel_lab;
    char *error;
} RvAsmResult;

void rvas_asm_result_free(RvAsmResult *r);

bool rvas_assemble(RvStmt **stmts, size_t n, RvAsmResult *out);

#endif
