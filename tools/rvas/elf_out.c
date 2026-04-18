#include "elf_out.h"
#include <elf.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

enum {
    RV_R_32 = 1,
    RV_R_64 = 2,
    RV_R_BRANCH = 16,
    RV_R_JAL = 17,
    RV_R_CALL_PLT = 19,
    RV_R_PCREL_HI20 = 23,
    RV_R_PCREL_LO12_I = 24,
    RV_R_RELAX = 33,
};



static size_t align_up(size_t x, size_t al) {
    return (x + al - 1) & ~(al - 1);
}

typedef struct {
    uint32_t name_off;
    uint8_t info;
    uint8_t other;
    uint16_t shndx;
    uint64_t value;
    uint64_t size;
} SymOut;

static int sym_idx_by_name(SymOut *sym_out, size_t sym_n, const char *name, const char *strtab) {
    for (size_t i = 0; i < sym_n; i++) {
        if (sym_out[i].name_off == 0) {
            if (name[0] == '\0')
                return (int)i;
            continue;
        }
        if (strcmp(strtab + sym_out[i].name_off, name) == 0)
            return (int)i;
    }
    return -1;
}

static bool append_rela(Elf64_Rela **r, size_t *n, size_t *cap, Elf64_Rela x) {
    if (*n >= *cap) {
        size_t nc = *cap ? *cap * 2 : 16;
        Elf64_Rela *p = realloc(*r, nc * sizeof(**r));
        if (!p)
            return false;
        *r = p;
        *cap = nc;
    }
    (*r)[(*n)++] = x;
    return true;
}

static SymOut *reorder_locals_first(SymOut *in, size_t sym_n, size_t *first_global_idx) {
    SymOut *out = calloc(sym_n, sizeof(*out));
    if (!out) {
        free(in);
        return NULL;
    }
    out[0] = in[0];
    size_t w = 1;
    for (size_t i = 1; i < sym_n; i++) {
        if ((in[i].info >> 4) == STB_LOCAL)
            out[w++] = in[i];
    }
    *first_global_idx = w;
    for (size_t i = 1; i < sym_n; i++) {
        if ((in[i].info >> 4) == STB_GLOBAL)
            out[w++] = in[i];
    }
    free(in);
    return out;
}

bool rvas_write_elf64_o(const RvAsmResult *a, FILE *out) {
    size_t str_cap = 256;
    for (size_t i = 0; i < a->sym_count; i++)
        str_cap += strlen(a->syms[i].name) + 1;
    for (size_t i = 0; i < a->fix_count; i++) {
        if (!a->fixups[i].patched && a->fixups[i].sym)
            str_cap += strlen(a->fixups[i].sym) + 1;
    }

    char *strtab = calloc(str_cap, 1);
    if (!strtab)
        return false;
    size_t str_pos = 1;

    size_t sym_n = 1 + a->sym_count;
    SymOut *sym_out = calloc(sym_n, sizeof(*sym_out));
    if (!sym_out) {
        free(strtab);
        return false;
    }
    sym_out[0] = (SymOut){0, 0, 0, SHN_UNDEF, 0, 0};

    for (size_t i = 0; i < a->sym_count; i++) {
        RvSym *s = &a->syms[i];
        size_t idx = 1 + i;
        sym_out[idx].name_off = (uint32_t)str_pos;
        memcpy(strtab + str_pos, s->name, strlen(s->name) + 1);
        str_pos += strlen(s->name) + 1;
        sym_out[idx].info = (uint8_t)((s->global ? STB_GLOBAL : STB_LOCAL) << 4 | STT_NOTYPE);
        sym_out[idx].other = STV_DEFAULT;
        if (!s->defined) {
            sym_out[idx].shndx = SHN_UNDEF;
            sym_out[idx].value = 0;
        } else if (s->sec == 1) {
            sym_out[idx].shndx = 1;
            sym_out[idx].value = s->value;
        } else if (s->sec == 2) {
            sym_out[idx].shndx = 2;
            sym_out[idx].value = s->value;
        } else if (s->sec == 3) {
            sym_out[idx].shndx = 3;
            sym_out[idx].value = s->value;
        } else {
            sym_out[idx].shndx = SHN_UNDEF;
            sym_out[idx].value = 0;
        }
    }

    for (size_t i = 0; i < a->fix_count; i++) {
        RvFixup *f = &a->fixups[i];
        if (f->patched || !f->sym)
            continue;
        if (sym_idx_by_name(sym_out, sym_n, f->sym, strtab) >= 0)
            continue;
        size_t nl = strlen(f->sym) + 1;
        if (str_pos + nl > str_cap) {
            str_cap = (str_cap + nl) * 2;
            char *ns = realloc(strtab, str_cap);
            if (!ns)
                goto fail;
            strtab = ns;
        }
        SymOut *nsym = realloc(sym_out, (sym_n + 1) * sizeof(*sym_out));
        if (!nsym)
            goto fail;
        sym_out = nsym;
        sym_out[sym_n].name_off = (uint32_t)str_pos;
        memcpy(strtab + str_pos, f->sym, nl);
        str_pos += nl;
        sym_out[sym_n].info = (uint8_t)((STB_GLOBAL << 4) | STT_NOTYPE);
        sym_out[sym_n].other = STV_DEFAULT;
        sym_out[sym_n].shndx = SHN_UNDEF;
        sym_out[sym_n].value = 0;
        sym_n++;
    }

    size_t first_global_after_reorder = sym_n;
    sym_out = reorder_locals_first(sym_out, sym_n, &first_global_after_reorder);
    if (!sym_out)
        goto fail;

    Elf64_Rela *rt = NULL, *rd = NULL;
    size_t rtn = 0, rdn = 0, rtcap = 0, rdcap = 0;

    for (size_t i = 0; i < a->fix_count; i++) {
        RvFixup *f = &a->fixups[i];
        if (f->patched)
            continue;
        int si = sym_idx_by_name(sym_out, sym_n, f->sym, strtab);
        if (si < 0)
            continue;
        uint32_t rtype = 0;
        if (f->kind == RV_FIX_BRANCH)
            rtype = RV_R_BRANCH;
        else if (f->kind == RV_FIX_JAL)
            rtype = RV_R_JAL;
        else if (f->kind == RV_FIX_CALL_PLT)
            rtype = RV_R_CALL_PLT;
        else if (f->kind == RV_FIX_HI20)
            rtype = RV_R_PCREL_HI20;
        else if (f->kind == RV_FIX_LO12)
            rtype = RV_R_PCREL_LO12_I;
        else if (f->kind == RV_FIX_ABS64)
            rtype = RV_R_64;
        else if (f->kind == RV_FIX_ABS32)
            rtype = RV_R_32;
        if (!rtype)
            continue;

        Elf64_Rela rel = {.r_offset = f->off,
                          .r_info = ELF64_R_INFO((uint64_t)si, rtype),
                          .r_addend = 0};
        if (f->sec == 1) {
            if (!append_rela(&rt, &rtn, &rtcap, rel))
                goto fail;
            if (rtype == RV_R_PCREL_HI20 || rtype == RV_R_PCREL_LO12_I || rtype == RV_R_CALL_PLT) {
                Elf64_Rela rx = {.r_offset = f->off,
                                 .r_info = ELF64_R_INFO(0, RV_R_RELAX),
                                 .r_addend = 0};
                if (!append_rela(&rt, &rtn, &rtcap, rx))
                    goto fail;
            }
        } else if (f->sec == 3) {
            if (!append_rela(&rd, &rdn, &rdcap, rel))
                goto fail;
        }

    }

    static const char shstr[] =
        "\0.text\0.rodata\0.data\0.shstrtab\0.strtab\0.symtab\0.rela.text\0.rela.data\0";
    size_t shstr_sz = sizeof(shstr);

    size_t off = sizeof(Elf64_Ehdr);
    off = align_up(off, 8);
    size_t off_text = off;
    off += a->text.len;
    off = align_up(off, 8);
    size_t off_rodata = off;
    off += a->rodata.len;
    off = align_up(off, 8);
    size_t off_data = off;
    off += a->data.len;
    off = align_up(off, 8);
    size_t off_symtab = off;
    size_t symtab_sz = sym_n * sizeof(Elf64_Sym);
    off += symtab_sz;
    off = align_up(off, 8);
    size_t off_strtab = off;
    size_t str_sz = str_pos;
    off += str_sz;
    off = align_up(off, 8);
    size_t off_shstrtab = off;
    off += shstr_sz;
    off = align_up(off, 8);
    size_t off_rela_text = off;
    size_t rela_text_sz = rtn * sizeof(Elf64_Rela);
    off += rela_text_sz;
    off = align_up(off, 8);
    size_t off_rela_data = off;
    size_t rela_data_sz = rdn * sizeof(Elf64_Rela);
    off += rela_data_sz;
    off = align_up(off, 8);
    size_t off_shdr = off;
    const int SHNUM = 9;
    size_t shdr_sz = SHNUM * sizeof(Elf64_Shdr);
    size_t total = off_shdr + shdr_sz;

    unsigned char *buf = calloc(total, 1);
    if (!buf)
        goto fail;

    Elf64_Ehdr *eh = (Elf64_Ehdr *)buf;
    memcpy(eh->e_ident, ELFMAG, SELFMAG);
    eh->e_ident[EI_CLASS] = ELFCLASS64;
    eh->e_ident[EI_DATA] = ELFDATA2LSB;
    eh->e_ident[EI_VERSION] = EV_CURRENT;
    eh->e_type = ET_REL;
    eh->e_machine = EM_RISCV;
    eh->e_version = EV_CURRENT;
    eh->e_shoff = off_shdr;
    eh->e_ehsize = sizeof(Elf64_Ehdr);
    eh->e_shentsize = sizeof(Elf64_Shdr);
    eh->e_shnum = SHNUM;
    eh->e_shstrndx = 4;
    eh->e_flags = 0x4; /* EF_RISCV_FLOAT_ABI_DOUBLE */

    memcpy(buf + off_text, a->text.data, a->text.len);
    memcpy(buf + off_rodata, a->rodata.data, a->rodata.len);
    memcpy(buf + off_data, a->data.data, a->data.len);

    for (size_t i = 0; i < sym_n; i++) {
        Elf64_Sym *es = (Elf64_Sym *)(buf + off_symtab + i * sizeof(Elf64_Sym));
        es->st_name = sym_out[i].name_off;
        es->st_info = sym_out[i].info;
        es->st_other = sym_out[i].other;
        es->st_shndx = sym_out[i].shndx;
        es->st_value = sym_out[i].value;
        es->st_size = sym_out[i].size;
    }

    memcpy(buf + off_strtab, strtab, str_sz);
    memcpy(buf + off_shstrtab, shstr, shstr_sz);
    if (rela_text_sz && rt)
        memcpy(buf + off_rela_text, rt, rela_text_sz);
    if (rela_data_sz && rd)
        memcpy(buf + off_rela_data, rd, rela_data_sz);

    Elf64_Shdr *sh = (Elf64_Shdr *)(buf + off_shdr);
    sh[1] = (Elf64_Shdr){.sh_name = 1,
                         .sh_type = SHT_PROGBITS,
                         .sh_flags = SHF_ALLOC | SHF_EXECINSTR,
                         .sh_offset = off_text,
                         .sh_size = a->text.len,
                         .sh_addralign = 4};
    sh[2] = (Elf64_Shdr){.sh_name = 7,
                         .sh_type = SHT_PROGBITS,
                         .sh_flags = SHF_ALLOC,
                         .sh_offset = off_rodata,
                         .sh_size = a->rodata.len,
                         .sh_addralign = 1};
    sh[3] = (Elf64_Shdr){.sh_name = 16,
                         .sh_type = SHT_PROGBITS,
                         .sh_flags = SHF_ALLOC | SHF_WRITE,
                         .sh_offset = off_data,
                         .sh_size = a->data.len,
                         .sh_addralign = 8};
    sh[4] = (Elf64_Shdr){.sh_name = 22,
                         .sh_type = SHT_STRTAB,
                         .sh_offset = off_shstrtab,
                         .sh_size = shstr_sz,
                         .sh_addralign = 1};
    sh[5] = (Elf64_Shdr){.sh_name = 32,
                         .sh_type = SHT_STRTAB,
                         .sh_offset = off_strtab,
                         .sh_size = str_sz,
                         .sh_addralign = 1};
    sh[6] = (Elf64_Shdr){.sh_name = 40,
                         .sh_type = SHT_SYMTAB,
                         .sh_offset = off_symtab,
                         .sh_size = symtab_sz,
                         .sh_link = 5,
                         .sh_info = (uint32_t)first_global_after_reorder,
                         .sh_addralign = 8,
                         .sh_entsize = sizeof(Elf64_Sym)};
    sh[7] = (Elf64_Shdr){.sh_name = 48,
                         .sh_type = SHT_RELA,
                         .sh_offset = off_rela_text,
                         .sh_size = rela_text_sz,
                         .sh_link = 6,
                         .sh_info = 1,
                         .sh_addralign = 8,
                         .sh_entsize = sizeof(Elf64_Rela)};
    sh[8] = (Elf64_Shdr){.sh_name = 59,
                         .sh_type = SHT_RELA,
                         .sh_offset = off_rela_data,
                         .sh_size = rela_data_sz,
                         .sh_link = 6,
                         .sh_info = 3,
                         .sh_addralign = 8,
                         .sh_entsize = sizeof(Elf64_Rela)};

    bool ok = fwrite(buf, 1, total, out) == total;
    free(buf);
    free(strtab);
    free(sym_out);
    free(rt);
    free(rd);
    return ok;

fail:
    free(strtab);
    free(sym_out);
    free(rt);
    free(rd);
    return false;
}
