#ifndef RVAS_LEXER_H
#define RVAS_LEXER_H

#include "rvas.h"
#include <stddef.h>

typedef enum {
    TOK_EOF = 0,
    TOK_EOL,
    TOK_IDENT,
    TOK_INT,
    TOK_STRING,
    TOK_COMMA,
    TOK_LPAREN,
    TOK_RPAREN,
    TOK_COLON,
    TOK_MINUS,
} RvTokenKind;

typedef struct {
    RvTokenKind kind;
    const char *ptr;
    size_t len;
    int64_t intval;
    char *strval;
} RvToken;

struct RvLexer {
    const char *src;
    size_t pos;
    size_t len;
    int line_no;
};

void rvas_lexer_init(RvLexer *L, const char *src, size_t len);
void rvas_lexer_line_start(RvLexer *L);
RvToken rvas_lexer_next(RvLexer *L);
void rvas_token_free_string(RvToken *t);

#endif
