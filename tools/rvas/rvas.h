#ifndef RVAS_H
#define RVAS_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

typedef struct RvLexer RvLexer;
typedef struct RvParser RvParser;
typedef struct RvAsmState RvAsmState;

typedef enum {
    RV_SEC_NONE = 0,
    RV_SEC_TEXT,
    RV_SEC_RODATA,
    RV_SEC_DATA,
} RvSectionKind;

typedef enum {
    RV_OP_REG,
    RV_OP_IMM,
    RV_OP_SYM,
    RV_OP_MEM,
} RvOperandKind;

typedef struct {
    RvOperandKind kind;
    int32_t imm;
    int reg;
    char *sym;
} RvOperand;

typedef enum {
    RV_STMT_LABEL,
    RV_STMT_DIR,
    RV_STMT_INSN,
} RvStmtKind;

typedef struct {
    RvStmtKind kind;
    char *label;
    char *dir;
    char **dir_args;
    size_t dir_argc;
    char *mnemonic;
    RvOperand *ops;
    size_t op_count;
    int line_no;
} RvStmt;

void rvas_stmt_free(RvStmt *s);
void rvas_stmt_list_free(RvStmt **list, size_t n);

#endif
