#ifndef RVLD_H
#define RVLD_H

#include <elf.h>
#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>

/* Internal representation of a section from a .o file */
typedef struct {
    char    *name;
    uint32_t type;
    uint64_t flags;
    uint64_t addr;       /* absolute vaddr assigned during layout */
    uint64_t offset;     /* file offset in source .o */
    uint64_t size;
    uint32_t link;
    uint32_t info;
    uint64_t addralign;
    uint64_t entsize;
    uint8_t *data;       /* section bytes (NULL for SHT_NOBITS) */
} Section;

/* An input relocatable file */
typedef struct {
    char         *filename;
    Elf64_Ehdr    ehdr;
    Section      *sections;
    int           section_count;
    Elf64_Sym    *symtab;           /* points into sections[symtab_section_idx].data */
    int           symbol_count;
    char         *strtab;           /* points into the strtab section linked by sh_link */
    uint32_t      strtab_size;
    int           symtab_section_idx;
} ObjectFile;

/* A resolved global symbol with its final virtual address */
typedef struct {
    char      *name;
    uint64_t   value;   /* final virtual address after layout */
    uint64_t   size;
    uint8_t    info;
    uint8_t    other;
    uint16_t   shndx;   /* section index inside obj */
    ObjectFile *obj;
} GlobalSymbol;

/* Linker state */
typedef struct {
    ObjectFile  **objs;
    int           obj_count;
    GlobalSymbol *globals;
    int           global_count;

    /* Library search */
    char        **search_paths;
    int           search_path_count;
    char        **lib_names;
    int           lib_name_count;

    /* Output segment virtual addresses and sizes */
    uint64_t text_vaddr;
    uint64_t data_vaddr;
    uint64_t bss_vaddr;

    uint64_t text_size;
    uint64_t data_size;
    uint64_t bss_size;
} LinkerCtx;

/* elf_parse.c */
ObjectFile *load_object_file(const char *filename);
ObjectFile *load_object_from_memory(const uint8_t *data, size_t size, const char *name);
void        free_object_file(ObjectFile *obj);

/* archive.c */
ObjectFile *search_and_load_from_archive(const char *archive_path, const char *symbol_name);
bool load_archives_to_resolve(LinkerCtx *ctx);

/* linker.c */
bool collect_definitions(LinkerCtx *ctx);
bool resolve_symbols(LinkerCtx *ctx);
bool check_undefined_symbols(LinkerCtx *ctx);
bool layout_sections(LinkerCtx *ctx, uint64_t base_addr);

/* reloc.c */
bool apply_relocations(LinkerCtx *ctx);

/* elf_write.c */
bool write_executable(LinkerCtx *ctx, const char *out_filename);

#endif /* RVLD_H */
