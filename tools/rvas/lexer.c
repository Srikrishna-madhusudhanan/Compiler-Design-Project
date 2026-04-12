#include "lexer.h"
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

void rvas_lexer_init(RvLexer *L, const char *src, size_t len) {
    L->src = src;
    L->pos = 0;
    L->len = len;
    L->line_no = 1;
}

void rvas_lexer_line_start(RvLexer *L) {
    while (L->pos < L->len) {
        char c = L->src[L->pos];
        if (c == ' ' || c == '\t')
            L->pos++;
        else
            break;
    }
}

static bool is_ident_start(char c) {
    return isalpha((unsigned char)c) || c == '_' || c == '.' || c == '$';
}

static bool is_ident_cont(char c) {
    return isalnum((unsigned char)c) || c == '_' || c == '.' || c == '$';
}

static RvToken tok_simple(RvTokenKind k, RvLexer *L, size_t start) {
    RvToken t = {k, L->src + start, L->pos - start, 0, NULL};
    return t;
}

RvToken rvas_lexer_next(RvLexer *L) {
    rvas_lexer_line_start(L);
    size_t start = L->pos;
    if (L->pos >= L->len)
        return tok_simple(TOK_EOF, L, start);

    char c = L->src[L->pos];

    if (c == '#') {
        while (L->pos < L->len && L->src[L->pos] != '\n')
            L->pos++;
        if (L->pos < L->len && L->src[L->pos] == '\n') {
            L->pos++;
            L->line_no++;
        }
        rvas_lexer_line_start(L);
        return rvas_lexer_next(L);
    }

    if (c == '\n') {
        L->pos++;
        L->line_no++;
        return tok_simple(TOK_EOL, L, start);
    }
    if (c == '\r') {
        L->pos++;
        if (L->pos < L->len && L->src[L->pos] == '\n') {
            L->pos++;
            L->line_no++;
        }
        return tok_simple(TOK_EOL, L, start);
    }

    if (c == ',') {
        L->pos++;
        return tok_simple(TOK_COMMA, L, start);
    }
    if (c == '(') {
        L->pos++;
        return tok_simple(TOK_LPAREN, L, start);
    }
    if (c == ')') {
        L->pos++;
        return tok_simple(TOK_RPAREN, L, start);
    }
    if (c == ':') {
        L->pos++;
        return tok_simple(TOK_COLON, L, start);
    }
    if (c == '-') {
        L->pos++;
        return tok_simple(TOK_MINUS, L, start);
    }

    if (c == '"') {
        L->pos++;
        size_t cap = 32, slen = 0;
        char *buf = malloc(cap);
        if (!buf)
            return tok_simple(TOK_EOF, L, start);
        while (L->pos < L->len && L->src[L->pos] != '"') {
            if (L->src[L->pos] == '\\' && L->pos + 1 < L->len) {
                L->pos++;
                char e = L->src[L->pos++];
                if (e == 'n')
                    buf[slen++] = '\n';
                else if (e == 't')
                    buf[slen++] = '\t';
                else if (e == 'r')
                    buf[slen++] = '\r';
                else if (e == '0')
                    buf[slen++] = '\0';
                else
                    buf[slen++] = e;
            } else {
                if (slen + 1 >= cap) {
                    cap *= 2;
                    buf = realloc(buf, cap);
                    if (!buf)
                        return tok_simple(TOK_EOF, L, start);
                }
                buf[slen++] = L->src[L->pos++];
            }
        }
        if (L->pos < L->len && L->src[L->pos] == '"')
            L->pos++;
        buf[slen++] = '\0';
        RvToken t = {TOK_STRING, L->src + start, L->pos - start, 0, buf};
        return t;
    }

    if (isdigit((unsigned char)c) || (c == '0' && L->pos + 1 < L->len &&
                                        (L->src[L->pos + 1] == 'x' || L->src[L->pos + 1] == 'X'))) {
        int base = 10;
        if (c == '0' && L->pos + 1 < L->len &&
            (L->src[L->pos + 1] == 'x' || L->src[L->pos + 1] == 'X')) {
            L->pos += 2;
            base = 16;
        }
        int64_t v = 0;
        while (L->pos < L->len) {
            char d = L->src[L->pos];
            int dig = -1;
            if (d >= '0' && d <= '9')
                dig = d - '0';
            else if (base == 16 && d >= 'a' && d <= 'f')
                dig = 10 + (d - 'a');
            else if (base == 16 && d >= 'A' && d <= 'F')
                dig = 10 + (d - 'A');
            if (dig < 0 || dig >= base)
                break;
            v = v * base + dig;
            L->pos++;
        }
        RvToken t = {TOK_INT, L->src + start, L->pos - start, v, NULL};
        return t;
    }

    if (is_ident_start(c)) {
        L->pos++;
        while (L->pos < L->len && is_ident_cont(L->src[L->pos]))
            L->pos++;
        return tok_simple(TOK_IDENT, L, start);
    }

    L->pos++;
    return tok_simple(TOK_EOF, L, start);
}

void rvas_token_free_string(RvToken *t) {
    if (t->strval) {
        free(t->strval);
        t->strval = NULL;
    }
}
