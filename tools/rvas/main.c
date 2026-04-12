#include "parse.h"
#include "asm.h"
#include "elf_out.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static char *read_file(const char *path, size_t *out_len) {
    FILE *f = fopen(path, "rb");
    if (!f)
        return NULL;
    fseek(f, 0, SEEK_END);
    long n = ftell(f);
    fseek(f, 0, SEEK_SET);
    if (n < 0) {
        fclose(f);
        return NULL;
    }
    char *b = malloc((size_t)n + 1);
    if (!b) {
        fclose(f);
        return NULL;
    }
    size_t r = fread(b, 1, (size_t)n, f);
    fclose(f);
    b[r] = '\0';
    *out_len = r;
    return b;
}

int main(int argc, char **argv) {
    const char *inpath = NULL;
    const char *outpath = NULL;
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-o") == 0 && i + 1 < argc) {
            outpath = argv[++i];
        } else if (!inpath)
            inpath = argv[i];
        else {
            fprintf(stderr, "usage: rvas [-o out.o] file.s\n");
            return 1;
        }
    }
    if (!inpath) {
        fprintf(stderr, "usage: rvas [-o out.o] file.s\n");
        return 1;
    }

    size_t len = 0;
    char *src = read_file(inpath, &len);
    if (!src) {
        perror(inpath);
        return 1;
    }

    RvParseResult pr = rvas_parse_file(src, len);
    free(src);
    if (pr.error) {
        fprintf(stderr, "parse: %s\n", pr.error);
        rvas_parse_result_free(&pr);
        return 1;
    }

    RvAsmResult ar;
    if (!rvas_assemble(pr.stmts, pr.count, &ar)) {
        fprintf(stderr, "assemble: %s\n", ar.error ? ar.error : "?");
        rvas_parse_result_free(&pr);
        rvas_asm_result_free(&ar);
        return 1;
    }
    rvas_parse_result_free(&pr);

    FILE *out = stdout;
    if (outpath) {
        out = fopen(outpath, "wb");
        if (!out) {
            perror(outpath);
            rvas_asm_result_free(&ar);
            return 1;
        }
    }

    if (!rvas_write_elf64_o(&ar, out)) {
        fprintf(stderr, "elf: write failed\n");
        if (out != stdout)
            fclose(out);
        rvas_asm_result_free(&ar);
        return 1;
    }

    if (out != stdout)
        fclose(out);
    rvas_asm_result_free(&ar);
    return 0;
}
