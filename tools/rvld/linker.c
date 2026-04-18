/*
 * linker.c — Symbol resolution and section layout
 */
#include "rvld.h"
#include <stdlib.h>
#include <string.h>

/* -------------------------------------------------------------------------
 * Symbol resolution
 * Phase 1: collect all global DEFINED symbols.
 * Phase 2: verify every global UNDEFINED reference is satisfied.
 * ------------------------------------------------------------------------- */
bool resolve_symbols(LinkerCtx *ctx) {
    /* Phase 1: definitions */
    for (int i = 0; i < ctx->obj_count; i++) {
        ObjectFile *obj = ctx->objs[i];
        if (!obj->symtab || !obj->strtab) continue;

        for (int j = 1; j < obj->symbol_count; j++) {   /* skip sym 0 (null) */
            Elf64_Sym *sym = &obj->symtab[j];
            if (ELF64_ST_BIND(sym->st_info) != STB_GLOBAL) continue;
            if (sym->st_shndx == SHN_UNDEF) continue;   /* undefined — skip */

            const char *name = obj->strtab + sym->st_name;

            /* Check for duplicate definition */
            for (int k = 0; k < ctx->global_count; k++) {
                if (strcmp(ctx->globals[k].name, name) == 0) {
                    fprintf(stderr,
                        "rvld: error: multiple definition of '%s'\n"
                        "       first defined in %s\n"
                        "       redefined  in   %s\n",
                        name,
                        ctx->globals[k].obj->filename,
                        obj->filename);
                    return false;
                }
            }

            /* Grow global table */
            GlobalSymbol *ng = realloc(ctx->globals,
                                       (ctx->global_count + 1) * sizeof(GlobalSymbol));
            if (!ng) { perror("realloc"); return false; }
            ctx->globals = ng;

            GlobalSymbol *gs = &ctx->globals[ctx->global_count++];
            gs->name  = strdup(name);
            gs->value = sym->st_value;  /* will be updated by layout_sections */
            gs->size  = sym->st_size;
            gs->info  = sym->st_info;
            gs->other = sym->st_other;
            gs->shndx = sym->st_shndx;
            gs->obj   = obj;
        }
    }

    /* Phase 2: undefined references */
    bool ok = true;
    for (int i = 0; i < ctx->obj_count; i++) {
        ObjectFile *obj = ctx->objs[i];
        if (!obj->symtab || !obj->strtab) continue;

        for (int j = 1; j < obj->symbol_count; j++) {
            Elf64_Sym *sym = &obj->symtab[j];
            if (ELF64_ST_BIND(sym->st_info) != STB_GLOBAL) continue;
            if (sym->st_shndx != SHN_UNDEF) continue;

            const char *name = obj->strtab + sym->st_name;
            bool found = false;
            for (int k = 0; k < ctx->global_count; k++) {
                if (strcmp(ctx->globals[k].name, name) == 0) { found = true; break; }
            }
            if (!found) {
                fprintf(stderr,
                    "rvld: error: undefined reference to '%s' (in %s)\n",
                    name, obj->filename);
                ok = false;
            }
        }
    }
    return ok;
}

/* -------------------------------------------------------------------------
 * Section layout
 * Assigns a virtual address (s->addr) to every relevant section.
 * Layout:
 *   [base_addr]         .text  (RX)
 *   [next 4K page]      .rodata  .data  (RW)
 *   [continued]         .bss  (zero-initialized, RW)
 * ------------------------------------------------------------------------- */
static uint64_t align_to(uint64_t v, uint64_t a) {
    if (a <= 1) return v;
    return (v + a - 1) & ~(a - 1);
}

bool layout_sections(LinkerCtx *ctx, uint64_t base_addr) {
    uint64_t header_sz = sizeof(Elf64_Ehdr) + 2 * sizeof(Elf64_Phdr);
    uint64_t pc = base_addr + header_sz;

    /* ---- .text ---------------------------------------------------------- */
    ctx->text_vaddr = base_addr;
    for (int i = 0; i < ctx->obj_count; i++) {
        ObjectFile *obj = ctx->objs[i];
        for (int j = 0; j < obj->section_count; j++) {
            Section *s = &obj->sections[j];
            if (s->type != SHT_PROGBITS) continue;
            if (!(s->flags & SHF_EXECINSTR)) continue;
            pc       = align_to(pc, s->addralign > 0 ? s->addralign : 4);
            s->addr  = pc;
            pc      += s->size;
        }
    }
    ctx->text_size = pc - ctx->text_vaddr;


    /* ---- .rodata and .data (same RW segment) ---------------------------- */
    pc = align_to(pc, 0x1000);        /* put data on a new page */
    ctx->data_vaddr = pc;
    for (int i = 0; i < ctx->obj_count; i++) {
        ObjectFile *obj = ctx->objs[i];
        for (int j = 0; j < obj->section_count; j++) {
            Section *s = &obj->sections[j];
            if (s->type != SHT_PROGBITS) continue;
            if (s->flags & SHF_EXECINSTR) continue;
            if (!(s->flags & SHF_ALLOC)) continue;
            pc       = align_to(pc, s->addralign > 0 ? s->addralign : 1);
            s->addr  = pc;
            pc      += s->size;
        }
    }
    ctx->data_size = pc - ctx->data_vaddr;

    /* ---- .bss ----------------------------------------------------------- */
    ctx->bss_vaddr = pc;
    for (int i = 0; i < ctx->obj_count; i++) {
        ObjectFile *obj = ctx->objs[i];
        for (int j = 0; j < obj->section_count; j++) {
            Section *s = &obj->sections[j];
            if (s->type != SHT_NOBITS) continue;
            if (!(s->flags & SHF_ALLOC)) continue;
            pc       = align_to(pc, s->addralign > 0 ? s->addralign : 1);
            s->addr  = pc;
            pc      += s->size;
        }
    }
    ctx->bss_size = pc - ctx->bss_vaddr;

    /* ---- Finalize global symbol addresses ------------------------------- */
    for (int i = 0; i < ctx->global_count; i++) {
        GlobalSymbol *gs = &ctx->globals[i];
        int shndx = (int)gs->shndx;
        if (shndx < gs->obj->section_count) {
            gs->value = gs->obj->sections[shndx].addr + gs->value;
        }
    }

    return true;
}
