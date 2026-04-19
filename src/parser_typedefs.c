#include "parser_typedefs.h"

#include <stdlib.h>
#include <string.h>

typedef struct TypedefName {
    char *name;
    struct TypedefName *next;
} TypedefName;

static TypedefName *g_typedef_names = NULL;

int parser_is_typedef_name(const char *name) {
    if (!name) return 0;
    for (TypedefName *n = g_typedef_names; n; n = n->next) {
        if (strcmp(n->name, name) == 0) return 1;
    }
    return 0;
}

void parser_register_typedef_name(const char *name) {
    if (!name || !*name) return;
    if (parser_is_typedef_name(name)) return;

    TypedefName *n = (TypedefName*)malloc(sizeof(TypedefName));
    if (!n) return;
    n->name = strdup(name);
    n->next = g_typedef_names;
    g_typedef_names = n;
}

void parser_clear_typedef_names(void) {
    while (g_typedef_names) {
        TypedefName *n = g_typedef_names;
        g_typedef_names = g_typedef_names->next;
        free(n->name);
        free(n);
    }
}
