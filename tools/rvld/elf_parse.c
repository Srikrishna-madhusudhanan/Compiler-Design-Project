/*
 * elf_parse.c — Load ELF relocatable (.o) files
 * Uses sh_link to find the correct strtab for each symtab.
 */
#include "rvld.h"
#include <stdlib.h>
#include <string.h>
#include <errno.h>

ObjectFile *load_object_file(const char *filename) {
    FILE *f = fopen(filename, "rb");
    if (!f) {
        fprintf(stderr, "rvld: cannot open '%s': %s\n", filename, strerror(errno));
        return NULL;
    }

    ObjectFile *obj = calloc(1, sizeof(ObjectFile));
    if (!obj) { fclose(f); return NULL; }
    obj->filename = strdup(filename);

    /* --- Read ELF header ------------------------------------------------- */
    if (fread(&obj->ehdr, 1, sizeof(Elf64_Ehdr), f) != sizeof(Elf64_Ehdr)) {
        fprintf(stderr, "rvld: truncated ELF header in '%s'\n", filename);
        goto fail;
    }
    if (memcmp(obj->ehdr.e_ident, ELFMAG, SELFMAG) != 0) {
        fprintf(stderr, "rvld: '%s' is not an ELF file\n", filename);
        goto fail;
    }
    if (obj->ehdr.e_type != ET_REL) {
        fprintf(stderr, "rvld: '%s' is not a relocatable (.o) file\n", filename);
        goto fail;
    }

    /* --- Read all section headers ---------------------------------------- */
    int shnum = obj->ehdr.e_shnum;
    Elf64_Shdr *shdrs = calloc(shnum, sizeof(Elf64_Shdr));
    if (!shdrs) goto fail;

    fseek(f, (long)obj->ehdr.e_shoff, SEEK_SET);
    if (fread(shdrs, sizeof(Elf64_Shdr), shnum, f) != (size_t)shnum) {
        fprintf(stderr, "rvld: cannot read section headers in '%s'\n", filename);
        free(shdrs);
        goto fail;
    }

    /* --- Read shstrtab (section name string table) ----------------------- */
    Elf64_Shdr *shstr_sh = &shdrs[obj->ehdr.e_shstrndx];
    char *shstrtab = calloc(shstr_sh->sh_size + 1, 1);
    if (!shstrtab) { free(shdrs); goto fail; }
    fseek(f, (long)shstr_sh->sh_offset, SEEK_SET);
    fread(shstrtab, 1, shstr_sh->sh_size, f);

    /* --- Allocate section array ------------------------------------------ */
    obj->section_count = shnum;
    obj->sections = calloc(shnum, sizeof(Section));
    if (!obj->sections) { free(shstrtab); free(shdrs); goto fail; }

    /* --- Load each section ----------------------------------------------- */
    for (int i = 0; i < shnum; i++) {
        Section *s = &obj->sections[i];
        s->name      = strdup(shstrtab + shdrs[i].sh_name);
        s->type      = shdrs[i].sh_type;
        s->flags     = shdrs[i].sh_flags;
        s->size      = shdrs[i].sh_size;
        s->offset    = shdrs[i].sh_offset;
        s->link      = shdrs[i].sh_link;
        s->info      = shdrs[i].sh_info;
        s->addralign = shdrs[i].sh_addralign;
        s->entsize   = shdrs[i].sh_entsize;
        s->addr      = 0; /* filled during layout */

        if (s->size > 0 && shdrs[i].sh_type != SHT_NOBITS) {
            s->data = calloc(s->size, 1);
            if (!s->data) { free(shstrtab); free(shdrs); goto fail; }
            fseek(f, (long)shdrs[i].sh_offset, SEEK_SET);
            fread(s->data, 1, s->size, f);
        }
    }

    free(shstrtab);
    free(shdrs);

    /* --- Find symtab and its linked strtab ------------------------------- */
    for (int i = 0; i < shnum; i++) {
        Section *s = &obj->sections[i];
        if (s->type == SHT_SYMTAB) {
            obj->symtab_section_idx = i;
            obj->symtab       = (Elf64_Sym *)s->data;
            obj->symbol_count = (int)(s->size / sizeof(Elf64_Sym));
            /* sh_link points to the strtab this symtab uses */
            int strtab_idx = (int)s->link;
            if (strtab_idx > 0 && strtab_idx < shnum) {
                obj->strtab      = (char *)obj->sections[strtab_idx].data;
                obj->strtab_size = (uint32_t)obj->sections[strtab_idx].size;
            }
            break; /* only one symtab per .o */
        }
    }

    fclose(f);
    return obj;

fail:
    if (f) fclose(f);
    if (obj) free_object_file(obj);
    return NULL;
}

void free_object_file(ObjectFile *obj) {
    if (!obj) return;
    for (int i = 0; i < obj->section_count; i++) {
        free(obj->sections[i].name);
        if (obj->sections[i].data) free(obj->sections[i].data);
    }
    free(obj->sections);
    free(obj->filename);
    free(obj);
}
