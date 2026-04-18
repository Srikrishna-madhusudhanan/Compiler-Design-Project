#include "rvld.h"
#include <stdlib.h>
#include <string.h>

int main(int argc, char **argv) {
    if (argc < 4 || strcmp(argv[1], "-o") != 0) {
        fprintf(stderr, "Usage: %s -o <output> <input1> [<input2> ...]\n", argv[0]);
        return 1;
    }

    const char *out_filename = argv[2];
    LinkerCtx ctx = {0};
    ctx.obj_count = argc - 3;
    ctx.objs = calloc(ctx.obj_count, sizeof(ObjectFile *));

    for (int i = 0; i < ctx.obj_count; i++) {
        ctx.objs[i] = load_object_file(argv[i + 3]);
        if (!ctx.objs[i]) {
            fprintf(stderr, "rvld: failed to load %s\n", argv[i + 3]);
            return 1;
        }
    }

    if (!resolve_symbols(&ctx)) {
        fprintf(stderr, "rvld: symbol resolution failed\n");
        return 1;
    }

    // Base address 0x10000 for simple RISC-V QEMU/spike execution
    if (!layout_sections(&ctx, 0x10000)) {
        fprintf(stderr, "rvld: section layout failed\n");
        return 1;
    }

    if (!apply_relocations(&ctx)) {
        fprintf(stderr, "rvld: relocation application failed\n");
        return 1;
    }

    if (!write_executable(&ctx, out_filename)) {
        fprintf(stderr, "rvld: executable generation failed\n");
        return 1;
    }

    printf("rvld: successfully linked -> %s\n", out_filename);

    return 0;
}
