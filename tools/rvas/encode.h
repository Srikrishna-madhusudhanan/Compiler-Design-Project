#ifndef RVAS_ENCODE_H
#define RVAS_ENCODE_H

#include <stdint.h>

int rvas_reg_by_name(const char *name);

uint32_t rvas_pack_r(uint32_t funct7, uint32_t rs2, uint32_t rs1, uint32_t funct3, uint32_t rd,
                     uint32_t opcode);
uint32_t rvas_pack_i(int32_t imm12, uint32_t rs1, uint32_t funct3, uint32_t rd, uint32_t opcode);
uint32_t rvas_pack_s(int32_t imm12, uint32_t rs2, uint32_t rs1, uint32_t funct3, uint32_t opcode);
uint32_t rvas_pack_b(int32_t imm_byte, uint32_t rs2, uint32_t rs1, uint32_t funct3, uint32_t opcode);
uint32_t rvas_pack_u(int32_t imm20, uint32_t rd, uint32_t opcode);
uint32_t rvas_pack_j(int32_t imm_byte, uint32_t rd, uint32_t opcode);

#endif
