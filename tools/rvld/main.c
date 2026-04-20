#include "rvld.h"
#include <stdlib.h>
#include <string.h>

int main(int argc, char **argv) {
    if (argc < 3) {
        fprintf(stderr, "Usage: %s -o <output> <inputs...> [-L <path>...] [-l <lib>...]\n", argv[0]);
        return 1;
    }

    const char *out_filename = NULL;
    LinkerCtx ctx = {0};

    /* Basic argument parsing */
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-o") == 0) {
            if (++i < argc) out_filename = argv[i];
        } else if (strncmp(argv[i], "-L", 2) == 0) {
            const char *path = NULL;
            if (strlen(argv[i]) > 2) path = argv[i] + 2;
            else if (++i < argc) path = argv[i];
            if (path) {
                ctx.search_paths = realloc(ctx.search_paths, (ctx.search_path_count + 1) * sizeof(char *));
                ctx.search_paths[ctx.search_path_count++] = (char *)path;
            }
        } else if (strncmp(argv[i], "-l", 2) == 0) {
            const char *name = NULL;
            if (strlen(argv[i]) > 2) name = argv[i] + 2;
            else if (++i < argc) name = argv[i];
            if (name) {
                ctx.lib_names = realloc(ctx.lib_names, (ctx.lib_name_count + 1) * sizeof(char *));
                ctx.lib_names[ctx.lib_name_count++] = (char *)name;
            }
        } else {
            /* Input object file */
            ObjectFile *obj = load_object_file(argv[i]);
            if (!obj) return 1;
            ctx.objs = realloc(ctx.objs, (ctx.obj_count + 1) * sizeof(ObjectFile *));
            ctx.objs[ctx.obj_count++] = obj;
        }
    }

    if (!out_filename || ctx.obj_count == 0) {
        fprintf(stderr, "rvld: error: no output filename or no input files specified\n");
        return 1;
    }

    /* 1. Initial symbol collection */
    if (!resolve_symbols(&ctx)) {
        return 1;
    }

    /* 2. Iterative library resolution */
    if (!load_archives_to_resolve(&ctx)) {
        return 1;
    }

    /* 3. Final undefined check */
    if (!check_undefined_symbols(&ctx)) {
        return 1;
    }

    /* 4. Layout */
    if (!layout_sections(&ctx, 0x10000)) {
        return 1;
    }

    /* 5. Relocate */
    if (!apply_relocations(&ctx)) {
        return 1;
    }

    /* 6. Write */
    if (!write_executable(&ctx, out_filename)) {
        return 1;
    }

    printf("rvld: successfully linked -> %s\n", out_filename);
    return 0;
}
