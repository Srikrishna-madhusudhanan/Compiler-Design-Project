#include "rvld.h"
#include <stdlib.h>
#include <string.h>

static uint32_t patch_b_type(uint32_t insn, int32_t imm) {
    uint32_t bit12 = (imm >> 12) & 1;
    uint32_t bit11 = (imm >> 11) & 1;
    uint32_t bit10_5 = (imm >> 5) & 0x3f;
    uint32_t bit4_1 = (imm >> 1) & 0xf;
    insn &= ~0xfe000f80;
    insn |= (bit12 << 31) | (bit10_5 << 25) | (bit4_1 << 8) | (bit11 << 7);
    return insn;
}

static uint32_t patch_j_type(uint32_t insn, int32_t imm) {
    uint32_t bit20 = (imm >> 20) & 1;
    uint32_t bit19_12 = (imm >> 12) & 0xff;
    uint32_t bit11 = (imm >> 11) & 1;
    uint32_t bit10_1 = (imm >> 1) & 0x3ff;
    insn &= ~0xfffff000;
    insn |= (bit20 << 31) | (bit19_12 << 12) | (bit11 << 20) | (bit10_1 << 21);
    return insn;
}

static uint32_t patch_u_type(uint32_t insn, int32_t imm) {
    uint32_t val = (uint32_t)imm & 0xfffff000;
    insn &= ~0xfffff000;
    insn |= val;
    return insn;
}

static uint32_t patch_i_type(uint32_t insn, int32_t imm) {
    uint32_t val = (uint32_t)imm & 0xfff;
    insn &= ~0xfff00000;
    insn |= (val << 20);
    return insn;
}

bool apply_relocations(LinkerCtx *ctx) {
    for (int i = 0; i < ctx->obj_count; i++) {
        ObjectFile *obj = ctx->objs[i];
        for (int j = 0; j < obj->section_count; j++) {
            Section *s = &obj->sections[j];
            if (s->type != SHT_RELA) continue;

            Section *target = &obj->sections[s->info];
            if (!target->data) continue;

            Elf64_Rela *relas = (Elf64_Rela *)s->data;
            int rela_count = s->size / sizeof(Elf64_Rela);

            for (int k = 0; k < rela_count; k++) {
                Elf64_Rela *rel = &relas[k];
                int sym_idx = ELF64_R_SYM(rel->r_info);
                int type = ELF64_R_TYPE(rel->r_info);
                
                if (sym_idx >= obj->symbol_count) continue;
                Elf64_Sym *sym = &obj->symtab[sym_idx];
                
                uint64_t S = 0;
                if (ELF64_ST_BIND(sym->st_info) == STB_GLOBAL) {
                    char *name = obj->strtab + sym->st_name;
                    bool found = false;
                    for (int g = 0; g < ctx->global_count; g++) {
                        if (strcmp(ctx->globals[g].name, name) == 0) {
                            S = ctx->globals[g].value;
                            found = true;
                            break;
                        }
                    }
                    if (!found) {
                        fprintf(stderr, "rvld: error: relocation against undefined symbol '%s'\n", name);
                        return false;
                    }
                } else {
                    // Local symbol
                    if (sym->st_shndx < obj->section_count) {
                        S = obj->sections[sym->st_shndx].addr + sym->st_value;
                    } else if (sym->st_shndx == SHN_ABS) {
                        S = sym->st_value;
                    }
                }

                uint64_t P = target->addr + rel->r_offset;
                int64_t A = rel->r_addend;
                
                if (rel->r_offset + 4 > target->size) continue;
                uint8_t *patch_site = target->data + rel->r_offset;
                
                switch (type) {
                    case 1: // R_RISCV_32
                        if (rel->r_offset + 4 <= target->size)
                            *(uint32_t *)patch_site = (uint32_t)(S + A);
                        break;
                    case 2: // R_RISCV_64
                        if (rel->r_offset + 8 <= target->size)
                            *(uint64_t *)patch_site = S + A;
                        break;
                    case 16: // R_RISCV_BRANCH
                    {
                        int32_t imm = (int32_t)(S + A - P);
                        uint32_t insn = *(uint32_t *)patch_site;
                        *(uint32_t *)patch_site = patch_b_type(insn, imm);
                        break;
                    }
                    case 17: // R_RISCV_JAL
                    {
                        int32_t imm = (int32_t)(S + A - P);
                        uint32_t insn = *(uint32_t *)patch_site;
                        *(uint32_t *)patch_site = patch_j_type(insn, imm);
                        break;
                    }
                    case 18: // R_RISCV_CALL (Deprecated but let's handle as CALL_PLT)
                    case 19: // R_RISCV_CALL_PLT
                    {
                        int32_t imm = (int32_t)(S + A - P);
                        int32_t hi = (imm + 0x800) & ~0xfff;
                        int32_t lo = imm - hi;
                        
                        uint32_t auipc = *(uint32_t *)patch_site;
                        uint32_t jalr = *(uint32_t *)(patch_site + 4);
                        
                        *(uint32_t *)patch_site = patch_u_type(auipc, hi);
                        *(uint32_t *)(patch_site + 4) = patch_i_type(jalr, lo);
                        break;
                    }
                    case 23: // R_RISCV_PCREL_HI20
                    {
                        int32_t imm = (int32_t)(S + A - P);
                        int32_t hi = (imm + 0x800) & ~0xfff;
                        uint32_t insn = *(uint32_t *)patch_site;
                        *(uint32_t *)patch_site = patch_u_type(insn, hi);
                        break;
                    }
                    case 24: // R_RISCV_PCREL_LO12_I
                    {
                        uint64_t target_value = S;
                        if (ELF64_ST_BIND(sym->st_info) != STB_GLOBAL && sym->st_shndx < obj->section_count) {
                            // Local anchor symbol: find matching HI20 immediate source if present.
                            for (int m = 0; m < rela_count; m++) {
                                Elf64_Rela *r2 = &relas[m];
                                if (r2->r_offset + 4 != rel->r_offset)
                                    continue;
                                if (ELF64_R_TYPE(r2->r_info) != R_RISCV_PCREL_HI20)
                                    continue;
                                int sym_hi_idx = ELF64_R_SYM(r2->r_info);
                                if (sym_hi_idx < 0 || sym_hi_idx >= obj->symbol_count)
                                    break;
                                Elf64_Sym *sym_hi = &obj->symtab[sym_hi_idx];
                                if (ELF64_ST_BIND(sym_hi->st_info) == STB_GLOBAL) {
                                    char *name = obj->strtab + sym_hi->st_name;
                                    bool found = false;
                                    for (int g = 0; g < ctx->global_count; g++) {
                                        if (strcmp(ctx->globals[g].name, name) == 0) {
                                            target_value = ctx->globals[g].value;
                                            found = true;
                                            break;
                                        }
                                    }
                                    if (!found) {
                                        fprintf(stderr, "rvld: error: relocation against undefined symbol '%s'\n", name);
                                        return false;
                                    }
                                } else if (sym_hi->st_shndx < obj->section_count) {
                                    target_value = obj->sections[sym_hi->st_shndx].addr + sym_hi->st_value;
                                } else if (sym_hi->st_shndx == SHN_ABS) {
                                    target_value = sym_hi->st_value;
                                }
                                break;
                            }
                        }

                        int32_t imm = (int32_t)(target_value + A - (P - 4));
                        int32_t hi = (imm + 0x800) & ~0xfff;
                        int32_t lo = imm - hi;
                        uint32_t insn = *(uint32_t *)patch_site;
                        *(uint32_t *)patch_site = patch_i_type(insn, lo);
                        break;
                    }
                    case 33: // R_RISCV_RELAX
                        break;
                    default:
                        // Ignore unknown
                        break;
                }
            }
        }
    }
    return true;
}
