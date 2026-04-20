/*
 * elf_parse.c — Load ELF relocatable (.o) files
 * Uses sh_link to find the correct strtab for each symtab.
 */
#include "rvld.h"
#include <stdlib.h>
#include <string.h>
#include <errno.h>

static ObjectFile *load_object_internal(const uint8_t *buffer, size_t size, const char *name) {
    ObjectFile *obj = calloc(1, sizeof(ObjectFile));
    if (!obj) return NULL;
    obj->filename = strdup(name);

    if (size < sizeof(Elf64_Ehdr)) {
        fprintf(stderr, "rvld: truncated ELF header in '%s'\n", name);
        goto fail;
    }

    memcpy(&obj->ehdr, buffer, sizeof(Elf64_Ehdr));
    if (memcmp(obj->ehdr.e_ident, ELFMAG, SELFMAG) != 0) {
        fprintf(stderr, "rvld: '%s' is not an ELF file\n", name);
        goto fail;
    }
    if (obj->ehdr.e_type != ET_REL) {
        fprintf(stderr, "rvld: '%s' is not a relocatable (.o) file\n", name);
        goto fail;
    }

    int shnum = obj->ehdr.e_shnum;
    if (obj->ehdr.e_shoff + shnum * sizeof(Elf64_Shdr) > size) {
        fprintf(stderr, "rvld: truncated section headers in '%s'\n", name);
        goto fail;
    }

    Elf64_Shdr *shdrs = (Elf64_Shdr *)(buffer + obj->ehdr.e_shoff);
    Elf64_Shdr *shstr_sh = &shdrs[obj->ehdr.e_shstrndx];
    const char *shstrtab = (const char *)(buffer + shstr_sh->sh_offset);

    obj->section_count = shnum;
    obj->sections = calloc(shnum, sizeof(Section));
    if (!obj->sections) goto fail;

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

        if (s->size > 0 && shdrs[i].sh_type != SHT_NOBITS) {
            if (s->offset + s->size > size) {
                fprintf(stderr, "rvld: truncated section data in '%s'\n", name);
                goto fail;
            }
            s->data = calloc(s->size, 1);
            if (!s->data) goto fail;
            memcpy(s->data, buffer + s->offset, s->size);
        }
    }

    for (int i = 0; i < shnum; i++) {
        Section *s = &obj->sections[i];
        if (s->type == SHT_SYMTAB) {
            obj->symtab_section_idx = i;
            obj->symtab       = (Elf64_Sym *)s->data;
            obj->symbol_count = (int)(s->size / sizeof(Elf64_Sym));
            int strtab_idx = (int)s->link;
            if (strtab_idx > 0 && strtab_idx < shnum) {
                obj->strtab      = (char *)obj->sections[strtab_idx].data;
                obj->strtab_size = (uint32_t)obj->sections[strtab_idx].size;
            }
            break;
        }
    }

    return obj;

fail:
    if (obj) free_object_file(obj);
    return NULL;
}

ObjectFile *load_object_file(const char *filename) {
    FILE *f = fopen(filename, "rb");
    if (!f) {
        fprintf(stderr, "rvld: cannot open '%s': %s\n", filename, strerror(errno));
        return NULL;
    }
    fseek(f, 0, SEEK_END);
    size_t size = ftell(f);
    fseek(f, 0, SEEK_SET);
    uint8_t *buffer = malloc(size);
    if (!buffer) { fclose(f); return NULL; }
    if (fread(buffer, 1, size, f) != size) {
        fprintf(stderr, "rvld: failed to read '%s'\n", filename);
        free(buffer);
        fclose(f);
        return NULL;
    }
    fclose(f);
    ObjectFile *obj = load_object_internal(buffer, size, filename);
    free(buffer);
    return obj;
}

ObjectFile *load_object_from_memory(const uint8_t *data, size_t size, const char *name) {
    return load_object_internal(data, size, name);
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
