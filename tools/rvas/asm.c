#include "asm.h"
#include "encode.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

enum { SEC_UNDEF = 0, SEC_TEXT = 1, SEC_RODATA = 2, SEC_DATA = 3 };

static void buf_free(RvBuf *b) {
    free(b->data);
    b->data = NULL;
    b->len = b->cap = 0;
}

static bool buf_reserve(RvBuf *b, size_t need) {
    if (b->len + need <= b->cap)
        return true;
    size_t nc = b->cap ? b->cap * 2 : 256;
    while (nc < b->len + need)
        nc *= 2;
    uint8_t *p = realloc(b->data, nc);
    if (!p)
        return false;
    b->data = p;
    b->cap = nc;
    return true;
}

static bool buf_append(RvBuf *b, const void *p, size_t n) {
    if (!buf_reserve(b, n))
        return false;
    memcpy(b->data + b->len, p, n);
    b->len += n;
    return true;
}

static bool buf_u32le(RvBuf *b, uint32_t w) {
    return buf_append(b, &w, 4);
}

static RvBuf *cur_buf(RvAsmResult *a, RvSectionKind k) {
    if (k == RV_SEC_TEXT)
        return &a->text;
    if (k == RV_SEC_RODATA)
        return &a->rodata;
    if (k == RV_SEC_DATA)
        return &a->data;
    return NULL;
}

static RvSym *sym_find(RvAsmResult *a, const char *name) {
    for (size_t i = 0; i < a->sym_count; i++) {
        if (strcmp(a->syms[i].name, name) == 0)
            return &a->syms[i];
    }
    return NULL;
}

static RvSym *sym_get(RvAsmResult *a, const char *name) {
    RvSym *s = sym_find(a, name);
    if (s)
        return s;
    RvSym *ns = realloc(a->syms, (a->sym_count + 1) * sizeof(*ns));
    if (!ns)
        return NULL;
    a->syms = ns;
    s = &a->syms[a->sym_count++];
    memset(s, 0, sizeof(*s));
    s->name = strdup(name);
    s->sec = SEC_UNDEF;
    s->defined = false;
    return s;
}

static void sym_define(RvAsmResult *a, const char *name, int sec, uint64_t val) {
    RvSym *s = sym_get(a, name);
    if (!s)
        return;
    if (s->defined && (s->sec != sec || s->value != val)) {
        /* duplicate definition - ignore second or error in caller */
    }
    s->defined = true;
    s->sec = sec;
    s->value = val;
}

static bool add_fixup(RvAsmResult *a, RvFixKind k, int sec, size_t off, const char *sym) {
    RvFixup *nf = realloc(a->fixups, (a->fix_count + 1) * sizeof(*nf));
    if (!nf)
        return false;
    a->fixups = nf;
    a->fixups[a->fix_count] = (RvFixup){k, sec, off, strdup(sym), false};
    a->fix_count++;
    return true;
}

void rvas_asm_result_free(RvAsmResult *r) {
    buf_free(&r->text);
    buf_free(&r->rodata);
    buf_free(&r->data);
    for (size_t i = 0; i < r->sym_count; i++)
        free(r->syms[i].name);
    free(r->syms);
    for (size_t i = 0; i < r->fix_count; i++)
        free(r->fixups[i].sym);
    free(r->fixups);
    free(r->error);
    memset(r, 0, sizeof(*r));
}

#define OP_IMM 0x13
#define OP 0x33
#define LUI 0x37
#define AUIPC 0x17
#define JAL 0x6f
#define JALR 0x67
#define BRANCH 0x63
#define LOAD 0x03
#define STORE 0x23

static int sec_kind_to_num(RvSectionKind k) {
    if (k == RV_SEC_TEXT)
        return SEC_TEXT;
    if (k == RV_SEC_RODATA)
        return SEC_RODATA;
    if (k == RV_SEC_DATA)
        return SEC_DATA;
    return SEC_UNDEF;
}

static bool emit_u32(RvAsmResult *a, RvSectionKind sec, uint32_t w, char *err) {
    RvBuf *b = cur_buf(a, sec);
    if (!b) {
        snprintf(err, 256, "no section");
        return false;
    }
    return buf_u32le(b, w);
}

static size_t cur_off(RvAsmResult *a, RvSectionKind sec) {
    RvBuf *b = cur_buf(a, sec);
    return b ? b->len : 0;
}

static void pcrel_split(int64_t d, int32_t *hi20, int32_t *lo12) {
    int32_t lo = (int32_t)(d - ((((d + 0x800LL) >> 12) << 12)));
    *lo12 = lo;
    *hi20 = (int32_t)((d - lo) >> 12) & 0xfffff;
}

static bool emit_insn(RvAsmResult *a, RvSectionKind sec, uint32_t w, char *err) {
    return emit_u32(a, sec, w, err);
}

static bool emit_real(RvAsmResult *a, RvSectionKind sec, const char *m, RvOperand *o, int n, char *err) {
    if (strcmp(m, "add") == 0 && n == 3) {
        if (o[0].kind != RV_OP_REG || o[1].kind != RV_OP_REG || o[2].kind != RV_OP_REG)
            goto bad;
        return emit_insn(a, sec,
                         rvas_pack_r(0, o[2].reg, o[1].reg, 0, o[0].reg, OP), err);
    }
    if (strcmp(m, "sub") == 0 && n == 3) {
        if (o[0].kind != RV_OP_REG || o[1].kind != RV_OP_REG || o[2].kind != RV_OP_REG)
            goto bad;
        return emit_insn(a, sec,
                         rvas_pack_r(0x20, o[2].reg, o[1].reg, 0, o[0].reg, OP), err);
    }
    if (strcmp(m, "mul") == 0 && n == 3) {
        if (o[0].kind != RV_OP_REG || o[1].kind != RV_OP_REG || o[2].kind != RV_OP_REG)
            goto bad;
        return emit_insn(a, sec,
                         rvas_pack_r(1, o[2].reg, o[1].reg, 0, o[0].reg, OP), err);
    }
    if (strcmp(m, "div") == 0 && n == 3) {
        if (o[0].kind != RV_OP_REG || o[1].kind != RV_OP_REG || o[2].kind != RV_OP_REG)
            goto bad;
        return emit_insn(a, sec,
                         rvas_pack_r(1, o[2].reg, o[1].reg, 4, o[0].reg, OP), err);
    }
    if (strcmp(m, "rem") == 0 && n == 3) {
        if (o[0].kind != RV_OP_REG || o[1].kind != RV_OP_REG || o[2].kind != RV_OP_REG)
            goto bad;
        return emit_insn(a, sec,
                         rvas_pack_r(1, o[2].reg, o[1].reg, 6, o[0].reg, OP), err);
    }
    if (strcmp(m, "addi") == 0 && n == 3) {
        if (o[0].kind != RV_OP_REG || o[1].kind != RV_OP_REG || o[2].kind != RV_OP_IMM)
            goto bad;
        return emit_insn(a, sec,
                         rvas_pack_i(o[2].imm, o[1].reg, 0, o[0].reg, OP_IMM), err);
    }
    if (strcmp(m, "andi") == 0 && n == 3) {
        if (o[0].kind != RV_OP_REG || o[1].kind != RV_OP_REG || o[2].kind != RV_OP_IMM)
            goto bad;
        return emit_insn(a, sec,
                         rvas_pack_i(o[2].imm, o[1].reg, 7, o[0].reg, OP_IMM), err);
    }
    if (strcmp(m, "slli") == 0 && n == 3) {
        if (o[0].kind != RV_OP_REG || o[1].kind != RV_OP_REG || o[2].kind != RV_OP_IMM)
            goto bad;
        int sh = o[2].imm & 0x3f;
        return emit_insn(a, sec, rvas_pack_i(sh, o[1].reg, 1, o[0].reg, OP_IMM), err);
    }
    if (strcmp(m, "sltiu") == 0 && n == 3) {
        if (o[0].kind != RV_OP_REG || o[1].kind != RV_OP_REG || o[2].kind != RV_OP_IMM)
            goto bad;
        return emit_insn(a, sec,
                         rvas_pack_i(o[2].imm, o[1].reg, 3, o[0].reg, OP_IMM), err);
    }
    if (strcmp(m, "ld") == 0 && n == 2) {
        if (o[0].kind != RV_OP_REG || o[1].kind != RV_OP_MEM)
            goto bad;
        return emit_insn(a, sec,
                         rvas_pack_i(o[1].imm, o[1].reg, 3, o[0].reg, LOAD), err);
    }
    if (strcmp(m, "lw") == 0 && n == 2) {
        if (o[0].kind != RV_OP_REG || o[1].kind != RV_OP_MEM)
            goto bad;
        return emit_insn(a, sec,
                         rvas_pack_i(o[1].imm, o[1].reg, 2, o[0].reg, LOAD), err);
    }
    if (strcmp(m, "sd") == 0 && n == 2) {
        if (o[0].kind != RV_OP_REG || o[1].kind != RV_OP_MEM)
            goto bad;
        return emit_insn(a, sec,
                         rvas_pack_s(o[1].imm, o[0].reg, o[1].reg, 3, STORE), err);
    }
    if (strcmp(m, "sw") == 0 && n == 2) {
        if (o[0].kind != RV_OP_REG || o[1].kind != RV_OP_MEM)
            goto bad;
        return emit_insn(a, sec,
                         rvas_pack_s(o[1].imm, o[0].reg, o[1].reg, 2, STORE), err);
    }
    if (strcmp(m, "lui") == 0 && n == 2) {
        if (o[0].kind != RV_OP_REG || o[1].kind != RV_OP_IMM)
            goto bad;
        int32_t im = o[1].imm;
        int32_t lo = (im << 20) >> 20;
        int32_t hi = (im - lo) >> 12;
        return emit_insn(a, sec, rvas_pack_u(hi & 0xfffff, o[0].reg, LUI), err);
    }
    if (strcmp(m, "auipc") == 0 && n == 2) {
        if (o[0].kind != RV_OP_REG || o[1].kind != RV_OP_IMM)
            goto bad;
        return emit_insn(a, sec, rvas_pack_u(o[1].imm & 0xfffff, o[0].reg, AUIPC), err);
    }
    if (strcmp(m, "jalr") == 0 && n == 3) {
        /* jalr rd, rs1, imm12 (compiler style) */
        if (o[0].kind != RV_OP_REG || o[1].kind != RV_OP_REG || o[2].kind != RV_OP_IMM)
            goto bad;
        return emit_insn(a, sec,
                         rvas_pack_i(o[2].imm, o[1].reg, 0, o[0].reg, JALR), err);
    }
    if (strcmp(m, "jalr") == 0 && n == 2) {
        /* jalr rd, offset(rs1) */
        if (o[0].kind != RV_OP_REG || o[1].kind != RV_OP_MEM)
            goto bad;
        return emit_insn(a, sec,
                         rvas_pack_i(o[1].imm, o[1].reg, 0, o[0].reg, JALR), err);
    }
    snprintf(err, 256, "unknown or bad operands for %s", m);
    return false;
bad:
    snprintf(err, 256, "bad operands for %s", m);
    return false;
}

static bool emit_branch(RvAsmResult *a, RvSectionKind sec, uint32_t funct3, RvOperand *rs1,
                        RvOperand *rs2, const char *label, char *err) {
    if (rs1->kind != RV_OP_REG || rs2->kind != RV_OP_REG) {
        snprintf(err, 256, "branch regs");
        return false;
    }
    size_t off = cur_off(a, sec);
    if (!emit_insn(a, sec, rvas_pack_b(0, rs2->reg, rs1->reg, funct3, BRANCH), err))
        return false;
    if (!add_fixup(a, RV_FIX_BRANCH, sec, off, label))
        return false;
    return true;
}

static bool emit_jal_fixup(RvAsmResult *a, RvSectionKind sec, int rd, const char *label, char *err) {
    size_t off = cur_off(a, sec);
    if (!emit_insn(a, sec, rvas_pack_j(0, (uint32_t)rd, JAL), err))
        return false;
    if (!add_fixup(a, RV_FIX_JAL, sec, off, label))
        return false;
    return true;
}

static bool emit_stmt(RvAsmResult *a, RvSectionKind sec, RvStmt *st, char *err) {
    const char *m = st->mnemonic;
    RvOperand *o = st->ops;
    int n = (int)st->op_count;

    if (strcmp(m, "mv") == 0 && n == 2) {
        RvOperand x[3] = {o[0], o[1], {RV_OP_IMM, .imm = 0}};
        return emit_real(a, sec, "addi", x, 3, err);
    }
    if (strcmp(m, "neg") == 0 && n == 2) {
        RvOperand z = {RV_OP_REG, .imm = 0, .reg = 0};
        RvOperand x[3] = {o[0], z, o[1]};
        return emit_real(a, sec, "sub", x, 3, err);
    }
    if (strcmp(m, "seqz") == 0 && n == 2) {
        RvOperand imm = {RV_OP_IMM, .imm = 1};
        RvOperand x[3] = {o[0], o[1], imm};
        return emit_real(a, sec, "sltiu", x, 3, err);
    }
    if (strcmp(m, "j") == 0 && n == 1 && o[0].kind == RV_OP_SYM)
        return emit_jal_fixup(a, sec, 0, o[0].sym, err);
    if (strcmp(m, "jr") == 0 && n == 1 && o[0].kind == RV_OP_REG) {
        RvOperand x[3] = {{RV_OP_REG, .reg = 0}, o[0], {RV_OP_IMM, .imm = 0}};
        return emit_real(a, sec, "jalr", x, 3, err);
    }
    if (strcmp(m, "li") == 0 && n == 2 && o[0].kind == RV_OP_REG && o[1].kind == RV_OP_IMM) {
        int32_t im = o[1].imm;
        if (im >= -2048 && im <= 2047) {
            RvOperand z = {RV_OP_REG, .reg = 0};
            RvOperand x[3] = {o[0], z, o[1]};
            return emit_real(a, sec, "addi", x, 3, err);
        }
        int32_t lo = (im << 20) >> 20;
        int32_t hi = (im - lo) >> 12;
        if (!emit_insn(a, sec, rvas_pack_u(hi & 0xfffff, o[0].reg, LUI), err))
            return false;
        RvOperand loop = {RV_OP_IMM, .imm = lo};
        RvOperand x2[3] = {o[0], o[0], loop};
        return emit_real(a, sec, "addi", x2, 3, err);
    }
    /* bgt rs,rt,L -> blt rt,rs,L ; ble rs,rt,L -> bge rt,rs,L */
    if (strcmp(m, "bgt") == 0 && n == 3)
        return emit_branch(a, sec, 4, &o[1], &o[0], o[2].sym, err);
    if (strcmp(m, "ble") == 0 && n == 3)
        return emit_branch(a, sec, 5, &o[1], &o[0], o[2].sym, err);
    if (strcmp(m, "beq") == 0 && n == 3)
        return emit_branch(a, sec, 0, &o[0], &o[1], o[2].sym, err);
    if (strcmp(m, "bne") == 0 && n == 3)
        return emit_branch(a, sec, 1, &o[0], &o[1], o[2].sym, err);
    if (strcmp(m, "blt") == 0 && n == 3)
        return emit_branch(a, sec, 4, &o[0], &o[1], o[2].sym, err);
    if (strcmp(m, "bge") == 0 && n == 3)
        return emit_branch(a, sec, 5, &o[0], &o[1], o[2].sym, err);

    if (strcmp(m, "call") == 0 && n == 1 && o[0].kind == RV_OP_SYM) {
        size_t off_auipc = cur_off(a, sec);
        if (!emit_insn(a, sec, rvas_pack_u(0, 1, AUIPC), err))
            return false;
        if (!add_fixup(a, RV_FIX_CALL_PLT, sec, off_auipc, o[0].sym))
            return false;
        RvOperand ja[3] = {{RV_OP_REG, .reg = 1},
                           {RV_OP_REG, .reg = 1},
                           {RV_OP_IMM, .imm = 0}};
        if (!emit_real(a, sec, "jalr", ja, 3, err))
            return false;
        return true;
    }
    if (strcmp(m, "la") == 0 && n == 2 && o[0].kind == RV_OP_REG && o[1].kind == RV_OP_SYM) {
        char ilab[40];
        snprintf(ilab, sizeof ilab, ".Lp%u", a->pcrel_lab++);
        sym_define(a, ilab, sec_kind_to_num(sec), cur_off(a, sec));
        size_t off_auipc = cur_off(a, sec);
        if (!emit_insn(a, sec, rvas_pack_u(0, o[0].reg, AUIPC), err))
            return false;
        if (!add_fixup(a, RV_FIX_HI20, sec, off_auipc, o[1].sym))
            return false;
        size_t off_addi = cur_off(a, sec);
        RvOperand ad[3] = {o[0], o[0], {RV_OP_IMM, .imm = 0}};
        if (!emit_real(a, sec, "addi", ad, 3, err))
            return false;
        if (!add_fixup(a, RV_FIX_LO12, sec, off_addi, ilab))
            return false;
        return true;
    }

    if (strcmp(m, "jal") == 0 && n == 2 && o[0].kind == RV_OP_REG && o[1].kind == RV_OP_SYM)
        return emit_jal_fixup(a, sec, o[0].reg, o[1].sym, err);

    return emit_real(a, sec, m, o, n, err);
}

static bool pass2(RvAsmResult *a, char *err) {
    for (size_t i = 0; i < a->fix_count; i++) {
        RvFixup *f = &a->fixups[i];
        RvBuf *b = NULL;
        if (f->sec == SEC_TEXT)
            b = &a->text;
        else
            continue;
        if (f->off + 4 > b->len)
            continue;

        RvSym *s = sym_find(a, f->sym);

        if (f->kind == RV_FIX_BRANCH) {
            if (!s || !s->defined || s->sec != SEC_TEXT)
                continue;
            int64_t tgt = (int64_t)s->value;
            int64_t pc = (int64_t)f->off;
            int64_t delta = tgt - pc;
            if (delta < -8192 || delta > 8190 || (delta & 1)) {
                snprintf(err, 256, "branch out of range %s", f->sym);
                return false;
            }
            uint32_t w;
            memcpy(&w, b->data + f->off, 4);
            uint32_t rs2 = (w >> 20) & 31;
            uint32_t rs1 = (w >> 15) & 31;
            uint32_t f3 = (w >> 12) & 7;
            w = rvas_pack_b((int32_t)delta, rs2, rs1, f3, BRANCH);
            memcpy(b->data + f->off, &w, 4);
            f->patched = true;
        } else if (f->kind == RV_FIX_JAL) {
            if (!s || !s->defined || s->sec != SEC_TEXT)
                continue;
            int64_t delta = (int64_t)s->value - (int64_t)f->off;
            if (delta <= -(1 << 21) || delta >= (1 << 21) || (delta & 1)) {
                snprintf(err, 256, "jal out of range %s", f->sym);
                return false;
            }
            uint32_t w;
            memcpy(&w, b->data + f->off, 4);
            uint32_t rd = (w >> 7) & 31;
            w = rvas_pack_j((int32_t)delta, rd, JAL);
            memcpy(b->data + f->off, &w, 4);
            f->patched = true;
        } else if (f->kind == RV_FIX_CALL_PLT) {
            if (!s || !s->defined || s->sec != SEC_TEXT)
                continue;
            int64_t d = (int64_t)s->value - (int64_t)f->off;
            int32_t hi20, lo12;
            pcrel_split(d, &hi20, &lo12);
            uint32_t w_auipc, w_jalr;
            memcpy(&w_auipc, b->data + f->off, 4);
            memcpy(&w_jalr, b->data + f->off + 4, 4);
            uint32_t rd_a = (w_auipc >> 7) & 31;
            w_auipc = rvas_pack_u(hi20 & 0xfffff, rd_a, AUIPC);
            uint32_t rd_j = (w_jalr >> 7) & 31;
            uint32_t rs_j = (w_jalr >> 15) & 31;
            w_jalr = rvas_pack_i(lo12, rs_j, 0, rd_j, JALR);
            memcpy(b->data + f->off, &w_auipc, 4);
            memcpy(b->data + f->off + 4, &w_jalr, 4);
            f->patched = true;
        }
        /* PCREL_HI20 / LO12: always resolved by linker (LO12 pairs with HI20 via .Lp* anchor). */
    }
    return true;
}

bool rvas_assemble(RvStmt **stmts, size_t n, RvAsmResult *out) {
    char err[256] = {0};
    memset(out, 0, sizeof(*out));
    RvSectionKind cur = RV_SEC_TEXT;

    for (size_t i = 0; i < n; i++) {
        RvStmt *st = stmts[i];
        if (st->kind == RV_STMT_LABEL) {
            int sn = sec_kind_to_num(cur);
            sym_define(out, st->label, sn, cur_off(out, cur));
            continue;
        }
        if (st->kind == RV_STMT_DIR) {
            if (strcmp(st->dir, ".text") == 0)
                cur = RV_SEC_TEXT;
            else if (strcmp(st->dir, ".data") == 0)
                cur = RV_SEC_DATA;
            else if (strcmp(st->dir, ".section") == 0 && st->dir_argc >= 1) {
                if (strcmp(st->dir_args[0], ".rodata") == 0)
                    cur = RV_SEC_RODATA;
            } else if (strcmp(st->dir, ".globl") == 0 && st->dir_argc >= 1) {
                RvSym *g = sym_get(out, st->dir_args[0]);
                if (g)
                    g->global = true;
            } else if (strcmp(st->dir, ".asciz") == 0 && st->dir_argc >= 1) {
                RvBuf *b = cur_buf(out, cur);
                if (!buf_append(b, st->dir_args[0], strlen(st->dir_args[0]) + 1)) {
                    out->error = strdup("oom");
                    rvas_asm_result_free(out);
                    return false;
                }
            } else if (strcmp(st->dir, ".dword") == 0 && st->dir_argc >= 1) {
                RvBuf *b = cur_buf(out, cur);
                uint64_t z = 0;
                size_t off = b->len;
                if (!buf_append(b, &z, 8)) {
                    out->error = strdup("oom");
                    rvas_asm_result_free(out);
                    return false;
                }
                if (!add_fixup(out, RV_FIX_ABS64, sec_kind_to_num(cur), off, st->dir_args[0])) {
                    out->error = strdup("oom");
                    rvas_asm_result_free(out);
                    return false;
                }
            }
            continue;
        }
        if (st->kind == RV_STMT_INSN) {
            if (!emit_stmt(out, cur, st, err)) {
                out->error = strdup(err);
                rvas_asm_result_free(out);
                return false;
            }
        }
    }

    if (!pass2(out, err)) {
        out->error = strdup(err);
        rvas_asm_result_free(out);
        return false;
    }
    return true;
}
