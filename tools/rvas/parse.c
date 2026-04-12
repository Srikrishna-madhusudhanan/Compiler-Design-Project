#include "parse.h"
#include "lexer.h"
#include "encode.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

static char *xstrdup(const char *s, size_t n) {
    char *p = malloc(n + 1);
    if (!p)
        return NULL;
    memcpy(p, s, n);
    p[n] = '\0';
    return p;
}

static void push_stmt(RvParseResult *r, RvStmt *st) {
    if (r->count >= r->cap) {
        size_t nc = r->cap ? r->cap * 2 : 32;
        RvStmt **n = realloc(r->stmts, nc * sizeof(*n));
        if (!n) {
            rvas_stmt_free(st);
            free(st);
            return;
        }
        r->stmts = n;
        r->cap = nc;
    }
    r->stmts[r->count++] = st;
}

static void set_err(RvParseResult *r, const char *msg) {
    if (!r->error)
        r->error = strdup(msg);
}

static RvOperand *parse_operand(RvToken **cur, RvToken *end, bool *ok) {
    *ok = false;
    if (*cur >= end)
        return NULL;

    if ((*cur)->kind == TOK_IDENT) {
        RvOperand *o = calloc(1, sizeof(*o));
        o->kind = RV_OP_SYM;
        o->sym = xstrdup((*cur)->ptr, (*cur)->len);
        (*cur)++;
        *ok = true;
        return o;
    }

    if ((*cur)->kind == TOK_INT) {
        int64_t base = (*cur)->intval;
        (*cur)++;
        if (*cur < end && (*cur)->kind == TOK_LPAREN) {
            (*cur)++;
            if (*cur >= end || (*cur)->kind != TOK_IDENT)
                return NULL;
            RvOperand *o = calloc(1, sizeof(*o));
            o->kind = RV_OP_MEM;
            o->imm = (int32_t)base;
            o->reg = -1;
            o->sym = xstrdup((*cur)->ptr, (*cur)->len);
            (*cur)++;
            if (*cur >= end || (*cur)->kind != TOK_RPAREN) {
                free(o->sym);
                free(o);
                return NULL;
            }
            (*cur)++;
            *ok = true;
            return o;
        }
        RvOperand *o = calloc(1, sizeof(*o));
        o->kind = RV_OP_IMM;
        o->imm = (int32_t)base;
        (*cur)++;
        *ok = true;
        return o;
    }

    if ((*cur)->kind == TOK_MINUS) {
        (*cur)++;
        if (*cur >= end || (*cur)->kind != TOK_INT)
            return NULL;
        RvOperand *o = calloc(1, sizeof(*o));
        o->kind = RV_OP_IMM;
        o->imm = (int32_t)(-(*cur)->intval);
        (*cur)++;
        *ok = true;
        return o;
    }

    return NULL;
}

static bool append_dir_arg(RvStmt *st, char *s) {
    char **na = realloc(st->dir_args, (st->dir_argc + 1) * sizeof(*na));
    if (!na)
        return false;
    st->dir_args = na;
    st->dir_args[st->dir_argc++] = s;
    return true;
}

static RvStmt *parse_directive(RvToken *toks, size_t nt, int line_no, RvParseResult *r) {
    if (nt < 1 || toks[0].kind != TOK_IDENT || toks[0].ptr[0] != '.') {
        set_err(r, "bad directive");
        return NULL;
    }
    RvStmt *st = calloc(1, sizeof(*st));
    st->kind = RV_STMT_DIR;
    st->line_no = line_no;
    st->dir = xstrdup(toks[0].ptr, toks[0].len);

    if (strcmp(st->dir, ".asciz") == 0) {
        if (nt < 2 || toks[1].kind != TOK_STRING) {
            set_err(r, ".asciz needs string");
            rvas_stmt_free(st);
            free(st);
            return NULL;
        }
        if (!append_dir_arg(st, strdup(toks[1].strval))) {
            rvas_stmt_free(st);
            free(st);
            return NULL;
        }
        return st;
    }

    for (size_t i = 1; i < nt; i++) {
        if (toks[i].kind == TOK_COMMA)
            continue;
        char *arg = NULL;
        if (toks[i].kind == TOK_IDENT)
            arg = xstrdup(toks[i].ptr, toks[i].len);
        else if (toks[i].kind == TOK_INT) {
            char buf[32];
            snprintf(buf, sizeof buf, "%lld", (long long)toks[i].intval);
            arg = strdup(buf);
        } else {
            set_err(r, "bad directive arg");
            rvas_stmt_free(st);
            free(st);
            return NULL;
        }
        if (!append_dir_arg(st, arg)) {
            free(arg);
            rvas_stmt_free(st);
            free(st);
            return NULL;
        }
    }
    return st;
}

static RvStmt *parse_instruction(RvToken *toks, size_t nt, int line_no, RvParseResult *r) {
    if (nt < 1 || toks[0].kind != TOK_IDENT) {
        set_err(r, "expected mnemonic");
        return NULL;
    }
    RvStmt *st = calloc(1, sizeof(*st));
    st->kind = RV_STMT_INSN;
    st->line_no = line_no;
    st->mnemonic = xstrdup(toks[0].ptr, toks[0].len);

    size_t cap = 8;
    st->ops = calloc(cap, sizeof(*st->ops));
    if (!st->ops) {
        free(st);
        return NULL;
    }

    size_t i = 1;
    while (i < nt) {
        if (toks[i].kind == TOK_COMMA) {
            i++;
            continue;
        }
        RvToken *p = &toks[i];
        RvToken *endp = toks + nt;
        bool ok = false;
        RvOperand *o = parse_operand(&p, endp, &ok);
        if (!ok || !o) {
            set_err(r, "bad operand");
            rvas_stmt_free(st);
            free(st);
            return NULL;
        }
        if (o->kind == RV_OP_MEM) {
            int ri = rvas_reg_by_name(o->sym);
            if (ri < 0) {
                set_err(r, "bad base register in mem op");
                free(o->sym);
                free(o);
                rvas_stmt_free(st);
                free(st);
                return NULL;
            }
            free(o->sym);
            o->sym = NULL;
            o->reg = ri;
        } else if (o->kind == RV_OP_SYM) {
            int ri = rvas_reg_by_name(o->sym);
            if (ri >= 0) {
                free(o->sym);
                o->sym = NULL;
                o->kind = RV_OP_REG;
                o->reg = ri;
            }
        }

        if (st->op_count >= cap) {
            cap *= 2;
            RvOperand *no = realloc(st->ops, cap * sizeof(*no));
            if (!no) {
                if (o->sym)
                    free(o->sym);
                free(o);
                rvas_stmt_free(st);
                free(st);
                return NULL;
            }
            st->ops = no;
        }
        st->ops[st->op_count++] = *o;
        free(o);
        i = (size_t)(p - toks);
    }
    return st;
}

void rvas_stmt_free(RvStmt *s) {
    if (!s)
        return;
    free(s->label);
    free(s->dir);
    for (size_t i = 0; i < s->dir_argc; i++)
        free(s->dir_args[i]);
    free(s->dir_args);
    free(s->mnemonic);
    for (size_t i = 0; i < s->op_count; i++)
        if (s->ops[i].sym)
            free(s->ops[i].sym);
    free(s->ops);
}

void rvas_stmt_list_free(RvStmt **list, size_t n) {
    for (size_t i = 0; i < n; i++) {
        rvas_stmt_free(list[i]);
        free(list[i]);
    }
    free(list);
}

void rvas_parse_result_free(RvParseResult *r) {
    rvas_stmt_list_free(r->stmts, r->count);
    r->stmts = NULL;
    r->count = r->cap = 0;
    free(r->error);
    r->error = NULL;
}

static void parse_line(RvLexer *lex, RvParseResult *r) {
    RvToken toks[64];
    size_t nt = 0;
    int line_no = lex->line_no;
    for (;;) {
        RvToken t = rvas_lexer_next(lex);
        if (t.kind == TOK_EOF)
            break;
        if (t.kind == TOK_EOL)
            break;
        if (nt >= sizeof(toks) / sizeof(toks[0])) {
            set_err(r, "too many tokens on line");
            return;
        }
        toks[nt++] = t;
    }

    size_t i = 0;
    while (i < nt) {
        if (toks[i].kind != TOK_IDENT) {
            set_err(r, "expected label or directive or insn");
            return;
        }
        if (i + 1 < nt && toks[i + 1].kind == TOK_COLON) {
            RvStmt *lb = calloc(1, sizeof(*lb));
            lb->kind = RV_STMT_LABEL;
            lb->line_no = line_no;
            lb->label = xstrdup(toks[i].ptr, toks[i].len);
            push_stmt(r, lb);
            i += 2;
            continue;
        }
        if (toks[i].ptr[0] == '.') {
            /* ".foo" as insn mnemonic (e.g. ".LC0:" label handled above) */
            if (i + 1 < nt && toks[i + 1].kind == TOK_COLON) {
                RvStmt *lb = calloc(1, sizeof(*lb));
                if (!lb)
                    return;
                lb->kind = RV_STMT_LABEL;
                lb->line_no = line_no;
                lb->label = xstrdup(toks[i].ptr, toks[i].len);
                push_stmt(r, lb);
                i += 2;
                continue;
            }
            RvStmt *d = parse_directive(toks + i, nt - i, line_no, r);
            if (!d)
                return;
            push_stmt(r, d);
            return;
        }
        RvStmt *in = parse_instruction(toks + i, nt - i, line_no, r);
        if (!in)
            return;
        push_stmt(r, in);
        return;
    }
}

RvParseResult rvas_parse_file(const char *src, size_t len) {
    RvParseResult r = {0};
    RvLexer L;
    rvas_lexer_init(&L, src, len);

    while (L.pos < L.len) {
        rvas_lexer_line_start(&L);
        if (L.pos >= L.len)
            break;
        if (L.src[L.pos] == '\n') {
            L.pos++;
            L.line_no++;
            continue;
        }
        parse_line(&L, &r);
        if (r.error)
            break;
    }
    return r;
}
