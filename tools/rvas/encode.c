#include "encode.h"
#include <string.h>
#include <stdlib.h>

static int parse_xreg(const char *name) {
    if (name[0] != 'x' && name[0] != 'X')
        return -1;
    char *end = NULL;
    long v = strtol(name + 1, &end, 10);
    if (end == name + 1 || *end != '\0' || v < 0 || v > 31)
        return -1;
    return (int)v;
}

int rvas_reg_by_name(const char *name) {
    int x = parse_xreg(name);
    if (x >= 0)
        return x;
    static const struct {
        const char *n;
        int i;
    } tab[] = {{"zero", 0}, {"ra", 1},  {"sp", 2},  {"gp", 3},  {"tp", 4},  {"t0", 5},
               {"t1", 6},     {"t2", 7},  {"s0", 8},  {"fp", 8},  {"s1", 9},  {"a0", 10},
               {"a1", 11},    {"a2", 12}, {"a3", 13}, {"a4", 14}, {"a5", 15}, {"a6", 16},
               {"a7", 17},    {"s2", 18}, {"s3", 19}, {"s4", 20}, {"s5", 21}, {"s6", 22},
               {"s7", 23},    {"s8", 24}, {"s9", 25}, {"s10", 26}, {"s11", 27}, {"t3", 28},
               {"t4", 29},    {"t5", 30}, {"t6", 31}};
    for (size_t i = 0; i < sizeof(tab) / sizeof(tab[0]); i++) {
        if (strcmp(name, tab[i].n) == 0)
            return tab[i].i;
    }
    return -1;
}

uint32_t rvas_pack_r(uint32_t funct7, uint32_t rs2, uint32_t rs1, uint32_t funct3, uint32_t rd,
                     uint32_t opcode) {
    return funct7 << 25 | (rs2 & 31) << 20 | (rs1 & 31) << 15 | (funct3 & 7) << 12 | (rd & 31) << 7 |
           (opcode & 0x7f);
}

uint32_t rvas_pack_i(int32_t imm12, uint32_t rs1, uint32_t funct3, uint32_t rd, uint32_t opcode) {
    uint32_t imm = (uint32_t)imm12 & 0xfffu;
    return imm << 20 | (rs1 & 31) << 15 | (funct3 & 7) << 12 | (rd & 31) << 7 | (opcode & 0x7f);
}

uint32_t rvas_pack_s(int32_t imm12, uint32_t rs2, uint32_t rs1, uint32_t funct3, uint32_t opcode) {
    uint32_t u = (uint32_t)imm12 & 0xfffu;
    uint32_t imm11_5 = (u >> 5) & 0x7f;
    uint32_t imm4_0 = u & 0x1f;
    return imm11_5 << 25 | (rs2 & 31) << 20 | (rs1 & 31) << 15 | (funct3 & 7) << 12 |
           imm4_0 << 7 | (opcode & 0x7f);
}

uint32_t rvas_pack_b(int32_t imm_byte, uint32_t rs2, uint32_t rs1, uint32_t funct3, uint32_t opcode) {
    /* imm_* are bits of the signed byte offset (even); do not pre-divide by 2 */
    uint32_t imm = (uint32_t)imm_byte;
    uint32_t bit12 = (imm >> 12) & 1;
    uint32_t bit11 = (imm >> 11) & 1;
    uint32_t bit10_5 = (imm >> 5) & 0x3f;
    uint32_t bit4_1 = (imm >> 1) & 0xf;
    return bit12 << 31 | bit10_5 << 25 | (rs2 & 31) << 20 | (rs1 & 31) << 15 | (funct3 & 7) << 12 |
           bit4_1 << 8 | bit11 << 7 | (opcode & 0x7f);
}

uint32_t rvas_pack_u(int32_t imm20, uint32_t rd, uint32_t opcode) {
    uint32_t imm = (uint32_t)imm20 & 0xfffffu;
    return imm << 12 | (rd & 31) << 7 | (opcode & 0x7f);
}

uint32_t rvas_pack_j(int32_t imm_byte, uint32_t rd, uint32_t opcode) {
    int32_t off = imm_byte & ~1;
    uint32_t u = (uint32_t)off;
    uint32_t imm20 = (u >> 20) & 1;
    uint32_t imm19_12 = (u >> 12) & 0xff;
    uint32_t imm11 = (u >> 11) & 1;
    uint32_t imm10_1 = (u >> 1) & 0x3ff;
    return imm20 << 31 | imm19_12 << 12 | imm11 << 20 | imm10_1 << 21 | (rd & 31) << 7 |
           (opcode & 0x7f);
}
