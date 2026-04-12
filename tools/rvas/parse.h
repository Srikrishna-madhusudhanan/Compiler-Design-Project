#ifndef RVAS_PARSE_H
#define RVAS_PARSE_H

#include "rvas.h"
#include <stddef.h>

typedef struct {
    RvStmt **stmts;
    size_t count;
    size_t cap;
    char *error;
} RvParseResult;

RvParseResult rvas_parse_file(const char *src, size_t len);
void rvas_parse_result_free(RvParseResult *r);

#endif
