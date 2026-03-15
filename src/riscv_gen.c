#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ir.h"
#include "riscv_gen.h"

// --- Dynamic Stack Memory Tracker ---
typedef struct {
    char name[64];
    int offset;
} VarOffset;

static VarOffset var_offsets[256];
static int var_count = 0;
static int current_sp_offset = -32;

static void reset_offsets() {
    var_count = 0;
    current_sp_offset = -32; // Start assigning variables below the saved ra & s0
}

static int get_offset(const char *name) {
    // Check if we already assigned stack space to this variable
    for (int i = 0; i < var_count; i++) {
        if (strcmp(var_offsets[i].name, name) == 0) {
            return var_offsets[i].offset;
        }
    }
    // If not, assign it the next available slot on the stack
    strcpy(var_offsets[var_count].name, name);
    var_offsets[var_count].offset = current_sp_offset;
    current_sp_offset -= 4; // Allocate 4 bytes (1 word)
    return var_offsets[var_count++].offset;
}

// --- Helper to load an IR operand into a RISC-V register ---
static void load_operand(FILE *out, IROperand op, const char *reg) {
    if (op.is_const) {
        fprintf(out, "  li %s, %d\n", reg, op.const_val);
    } else if (op.name) {
        int off = get_offset(op.name);
        fprintf(out, "  lw %s, %d(s0)\n", reg, off);
    }
}

// --- Main Generator ---
void riscv_generate(IRProgram *prog, const char *filename) {
    FILE *out = fopen(filename, "w");
    if (!out) {
        perror("Failed to open output file");
        return;
    }

    fprintf(out, "  .text\n");
    fprintf(out, "  .globl main\n\n");

    IRFunc *func = prog->funcs;
    while (func) {
        reset_offsets(); // Reset stack map for the new function
        
        fprintf(out, "%s:\n", func->name);

        // --- PROLOGUE ---
        fprintf(out, "  # --- Prologue ---\n");
        // We allocate a large flat stack space (1024 bytes) to be safe for now
        fprintf(out, "  addi sp, sp, -1024\n"); 
        fprintf(out, "  sw ra, 1020(sp)\n");    
        fprintf(out, "  sw s0, 1016(sp)\n");    
        fprintf(out, "  addi s0, sp, 1024\n");  
        fprintf(out, "\n");

        // --- FUNCTION BODY ---
        IRInstr *instr = func->instrs;
        while (instr) {
            fprintf(out, "  # Line %d: ", instr->line);
            switch (instr->kind) {
                case IR_ASSIGN:
                    fprintf(out, "%s = ...\n", instr->result);
                    load_operand(out, instr->src, "t0");
                    fprintf(out, "  sw t0, %d(s0)\n", get_offset(instr->result));
                    break;

                case IR_BINOP:
                    fprintf(out, "%s = ... %c ...\n", instr->result, instr->binop);
                    load_operand(out, instr->left, "t0");
                    load_operand(out, instr->right, "t1");
                    
                    if (instr->binop == '+')      fprintf(out, "  add t2, t0, t1\n");
                    else if (instr->binop == '-') fprintf(out, "  sub t2, t0, t1\n");
                    else if (instr->binop == '*') fprintf(out, "  mul t2, t0, t1\n");
                    else if (instr->binop == '/') fprintf(out, "  div t2, t0, t1\n");
                    
                    fprintf(out, "  sw t2, %d(s0)\n", get_offset(instr->result));
                    break;

                case IR_IF:
                    fprintf(out, "if (...) goto %s\n", instr->label);
                    load_operand(out, instr->left, "t0");
                    load_operand(out, instr->right, "t1");
                    
                    switch (instr->relop) {
                        case IR_EQ: fprintf(out, "  beq t0, t1, %s\n", instr->label); break;
                        case IR_NE: fprintf(out, "  bne t0, t1, %s\n", instr->label); break;
                        case IR_LT: fprintf(out, "  blt t0, t1, %s\n", instr->label); break;
                        case IR_GT: fprintf(out, "  bgt t0, t1, %s\n", instr->label); break;
                        case IR_LE: fprintf(out, "  ble t0, t1, %s\n", instr->label); break;
                        case IR_GE: fprintf(out, "  bge t0, t1, %s\n", instr->label); break;
                        default: break;
                    }
                    break;

                case IR_GOTO:
                    fprintf(out, "goto %s\n", instr->label);
                    fprintf(out, "  j %s\n", instr->label);
                    break;

                case IR_LABEL:
                    fprintf(out, "Label %s\n", instr->label);
                    fprintf(out, "%s:\n", instr->label);
                    break;

                case IR_RETURN:
                    fprintf(out, "return\n");
                    if (instr->src.name || instr->src.is_const) {
                        load_operand(out, instr->src, "a0"); // Return values go in a0
                    }
                    // --- EPILOGUE ---
                    fprintf(out, "  lw ra, 1020(sp)\n");
                    fprintf(out, "  lw s0, 1016(sp)\n");
                    fprintf(out, "  addi sp, sp, 1024\n");
                    fprintf(out, "  jr ra\n");
                    break;

                default:
                    fprintf(out, "Unimplemented IR instruction\n");
                    break;
            }
            instr = instr->next;
        }

        // --- DEFAULT EPILOGUE ---
        fprintf(out, "\n  # --- Default Epilogue ---\n");
        fprintf(out, "  lw ra, 1020(sp)\n");
        fprintf(out, "  lw s0, 1016(sp)\n");
        fprintf(out, "  addi sp, sp, 1024\n");
        fprintf(out, "  jr ra\n\n");

        func = func->next;
    }

    fclose(out);
    printf("RISC-V assembly successfully written to %s\n", filename);
}   