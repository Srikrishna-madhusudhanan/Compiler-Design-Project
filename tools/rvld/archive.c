#include "rvld.h"
#include <stdlib.h>
#include <string.h>
#include <errno.h>

typedef struct {
    char name[16];
    char date[12];
    char uid[6];
    char gid[6];
    char mode[8];
    char size[10];
    char magic[2];
} ArHeader;

static size_t parse_ar_size(const char *s) {
    char tmp[11] = {0};
    memcpy(tmp, s, 10);
    return (size_t)strtol(tmp, NULL, 10);
}

ObjectFile *search_and_load_from_archive(const char *archive_path, const char *symbol_name) {
    FILE *f = fopen(archive_path, "rb");
    if (!f) return NULL;

    char magic[8];
    if (fread(magic, 1, 8, f) != 8 || memcmp(magic, "!<arch>\n", 8) != 0) {
        fclose(f);
        return NULL;
    }

    /* First pass: find string table (//) if it exists */
    char *strtab = NULL;
    size_t strtab_size = 0;
    ArHeader hdr;
    while (fread(&hdr, 1, sizeof(hdr), f) == sizeof(hdr)) {
        size_t size = parse_ar_size(hdr.size);
        if (memcmp(hdr.name, "//              ", 16) == 0) {
            strtab = malloc(size);
            fread(strtab, 1, size, f);
            strtab_size = size;
            break;
        }
        fseek(f, size + (size % 2), SEEK_CUR);
    }
    fseek(f, 8, SEEK_SET); /* Back to start (after magic) */

    while (fread(&hdr, 1, sizeof(hdr), f) == sizeof(hdr)) {
        size_t size = parse_ar_size(hdr.size);
        long start_pos = ftell(f);

        /* Skip symbol table (/) and string table (//) */
        if (hdr.name[0] != '/' && hdr.name[0] != ' ') {
            uint8_t *buffer = malloc(size);
            if (fread(buffer, 1, size, f) == size) {
                char full_name[512];
                char mem_name[256] = {0};
                
                if (hdr.name[0] == '/') {
                    /* Long name from string table /offset */
                    int offset = atoi(hdr.name + 1);
                    if (strtab && offset < strtab_size) {
                        char *src = strtab + offset;
                        char *dest = mem_name;
                        while (*src && *src != '/') *dest++ = *src++;
                        *dest = '\0';
                    } else {
                        strcpy(mem_name, "long_name_err");
                    }
                } else {
                    /* Short name name/ */
                    memcpy(mem_name, hdr.name, 16);
                    char *end = strchr(mem_name, '/');
                    if (end) *end = '\0';
                }
                
                snprintf(full_name, sizeof(full_name), "%s(%s)", archive_path, mem_name);

                ObjectFile *obj = load_object_from_memory(buffer, size, full_name);
                if (obj) {
                    for (int i = 1; i < obj->symbol_count; i++) {
                        Elf64_Sym *sym = &obj->symtab[i];
                        if (ELF64_ST_BIND(sym->st_info) == STB_GLOBAL && sym->st_shndx != SHN_UNDEF) {
                            if (strcmp(obj->strtab + sym->st_name, symbol_name) == 0) {
                                free(buffer);
                                if (strtab) free(strtab);
                                fclose(f);
                                return obj;
                            }
                        }
                    }
                    free_object_file(obj);
                }
            }
            free(buffer);
        }

        fseek(f, start_pos + size + (size % 2), SEEK_SET);
    }

    if (strtab) free(strtab);
    fclose(f);
    return NULL;
}

bool load_archives_to_resolve(LinkerCtx *ctx) {
    bool progress = true;
    while (progress) {
        progress = false;

        /* Check all undefined global symbols */
        for (int i = 0; i < ctx->obj_count; i++) {
            ObjectFile *obj = ctx->objs[i];
            for (int j = 1; j < obj->symbol_count; j++) {
                Elf64_Sym *sym = &obj->symtab[j];
                if (ELF64_ST_BIND(sym->st_info) != STB_GLOBAL || sym->st_shndx != SHN_UNDEF)
                    continue;

                const char *name = obj->strtab + sym->st_name;
                
                /* Already resolved? */
                bool resolved = false;
                for (int k = 0; k < ctx->global_count; k++) {
                    if (strcmp(ctx->globals[k].name, name) == 0) {
                        resolved = true;
                        break;
                    }
                }
                if (resolved) continue;

                /* Try libraries */
                for (int l = 0; l < ctx->lib_name_count; l++) {
                    for (int p = 0; p < ctx->search_path_count; p++) {
                        char path[1024];
                        snprintf(path, sizeof(path), "%s/lib%s.a", ctx->search_paths[p], ctx->lib_names[l]);
                        
                        ObjectFile *found = search_and_load_from_archive(path, name);
                        if (found) {
                            printf("rvld: loaded %s to resolve '%s'\n", found->filename, name);
                            ctx->objs = realloc(ctx->objs, (ctx->obj_count + 1) * sizeof(ObjectFile *));
                            ctx->objs[ctx->obj_count++] = found;
                            
                            if (!collect_definitions(ctx)) return false;
                            
                            progress = true;
                            goto next_iteration;
                        }
                    }
                }
            }
        }
        next_iteration:;
    }
    return true;
}
