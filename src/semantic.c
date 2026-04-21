#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "symbol_table.h"
#include "ast.h"
#include "semantic.h"
#include "y.tab.h"

static Symbol *current_function = NULL;
int semantic_errors = 0;
static int break_context_depth = 0;
static int loop_context_depth = 0;

static int current_local_offset = 0;

int get_type_size(DataType t, int pointer_level, Symbol *struct_def);

static Symbol *current_class = NULL;

static int min3(int a, int b, int c) {
    int m = (a < b) ? a : b;
    return (m < c) ? m : c;
}

static int edit_distance_ci(const char *a, const char *b) {
    int la = (int)strlen(a);
    int lb = (int)strlen(b);
    int *dp = (int*)malloc(sizeof(int) * (lb + 1));
    if (!dp) return 999;

    for (int j = 0; j <= lb; ++j) dp[j] = j;

    for (int i = 1; i <= la; ++i) {
        int prev = dp[0];
        dp[0] = i;
        for (int j = 1; j <= lb; ++j) {
            int old = dp[j];
            int ca = tolower((unsigned char)a[i - 1]);
            int cb = tolower((unsigned char)b[j - 1]);
            int cost = (ca == cb) ? 0 : 1;
            dp[j] = min3(dp[j] + 1, dp[j - 1] + 1, prev + cost);
            prev = old;
        }
    }

    int dist = dp[lb];
    free(dp);
    return dist;
}

static Symbol *find_closest_symbol_in_scope_chain(const char *name, SymbolKind kind, int *out_dist) {
    Symbol *best = NULL;
    int best_dist = 999;
    for (Scope *scope = current_scope; scope; scope = scope->parent) {
        for (int i = 0; i < TABLE_SIZE; ++i) {
            for (Symbol *sym = scope->table[i]; sym; sym = sym->next) {
                if (kind != (SymbolKind)-1 && sym->kind != kind) continue;
                int d = edit_distance_ci(name, sym->name);
                if (d < best_dist) {
                    best_dist = d;
                    best = sym;
                }
            }
        }
    }
    if (out_dist) *out_dist = best_dist;
    return best;
}

static const char *find_closest_builtin_name(const char *name, int *out_dist) {
    static const char *builtins[] = {
        "printf", "scanf", "malloc", "free", "NULL",
        "int", "char", "void", "typedef", "return",
        NULL
    };
    const char *best = NULL;
    int best_dist = 999;
    for (int i = 0; builtins[i]; ++i) {
        int d = edit_distance_ci(name, builtins[i]);
        if (d < best_dist) {
            best_dist = d;
            best = builtins[i];
        }
    }
    if (out_dist) *out_dist = best_dist;
    return best;
}

static void semantic_error_undeclared_with_hint(int line, const char *kind, const char *name, SymbolKind symbol_kind_hint) {
    char msg[512];
    int best_sym_dist = 999;
    Symbol *best_sym = find_closest_symbol_in_scope_chain(name, symbol_kind_hint, &best_sym_dist);
    int best_builtin_dist = 999;
    const char *best_builtin = find_closest_builtin_name(name, &best_builtin_dist);

    if (best_sym && best_sym_dist <= 2 && (!best_builtin || best_sym_dist <= best_builtin_dist)) {
        snprintf(msg, sizeof(msg), "Undeclared %s '%s'. Did you mean '%s'?", kind, name, best_sym->name);
    } else if (best_builtin && best_builtin_dist <= 2) {
        snprintf(msg, sizeof(msg), "Undeclared %s '%s'. Did you mean '%s'?", kind, name, best_builtin);
    } else {
        snprintf(msg, sizeof(msg), "Undeclared %s '%s'", kind, name);
    }

    semantic_error(line, msg);
}

static void resolve_decl_type(ASTNode *type_node,
                              int declarator_pointer,
                              int line,
                              DataType *out_type,
                              int *out_pointer,
                              Symbol **out_struct_def) {
    DataType type = TYPE_VOID;
    int pointer = declarator_pointer;
    Symbol *struct_def = NULL;

    if (type_node) {
        type = type_node->data_type;
        pointer += type_node->pointer_level;
    }

    if (type_node && type_node->str_val) {
        Symbol *s = lookup(type_node->str_val);
        if (s && s->kind == SYM_TYPEDEF) {
            type = s->type;
            pointer += s->pointer_level;
            struct_def = s->struct_def;
        } else if (type == TYPE_STRUCT) {
            if (s && s->kind == SYM_STRUCT) {
                struct_def = s;
            } else {
                semantic_error(line, "Unknown struct/typedef type");
            }
        }
    }

    if (out_type) *out_type = type;
    if (out_pointer) *out_pointer = pointer;
    if (out_struct_def) *out_struct_def = struct_def;
}

static Symbol *find_struct_member(Symbol *struct_sym, const char *name) {
    if (!struct_sym || struct_sym->kind != SYM_STRUCT) return NULL;
    Symbol *m = struct_sym->members;
    while (m) {
        if ((m->unmangled_name && strcmp(m->unmangled_name, name) == 0) || strcmp(m->name, name) == 0)
            return m;
        m = m->next_member;
    }
    /* Also check virtual methods */
    m = struct_sym->virtual_methods;
    while (m) {
        if ((m->unmangled_name && strcmp(m->unmangled_name, name) == 0) || strcmp(m->name, name) == 0)
            return m;
        m = m->next_member;
    }
    fprintf(stderr, "DEBUG: find_struct_member failed for '%s' in class '%s'. Members:\n", name, struct_sym->name);
    m = struct_sym->members;
    while (m) {
        fprintf(stderr, "  member name='%s' unmangled='%s' kind=%d\n", m->name, m->unmangled_name ? m->unmangled_name : "(null)", m->kind);
        m = m->next_member;
    }
    m = struct_sym->virtual_methods;
    while (m) {
        fprintf(stderr, "  virtual name='%s' unmangled='%s' kind=%d\n", m->name, m->unmangled_name ? m->unmangled_name : "(null)", m->kind);
        m = m->next_member;
    }
    return NULL;
}

static Symbol *find_virtual_method(Symbol *struct_sym, const char *name) {
    Symbol *m = struct_sym->virtual_methods;
    while (m) {
        if ((m->unmangled_name && strcmp(m->unmangled_name, name) == 0) || strcmp(m->name, name) == 0)
            return m;
        m = m->next_virtual;
    }
    return NULL;
}

static void replace_virtual_method(Symbol *struct_sym, Symbol *new_method) {
    Symbol **m = &struct_sym->virtual_methods;
    while (*m) {
        if (((*m)->unmangled_name && new_method->unmangled_name && strcmp((*m)->unmangled_name, new_method->unmangled_name) == 0) ||
            strcmp((*m)->name, new_method->name) == 0) {
            new_method->next_virtual = (*m)->next_virtual;
            *m = new_method;
            return;
        }
        m = &(*m)->next_virtual;
    }
}

void semantic_error(int line, const char *msg) {
    printf("Semantic Error (line %d): %s\n", line, msg);
    semantic_errors++;
}

/* Check if a type represents a void pointer (void*) */
static int is_void_pointer(DataType type, int pointer_level) {
    return type == TYPE_VOID && pointer_level > 0;
}

/* Check if a node represents the NULL constant */
static int is_null_constant(ASTNode *node) {
    return node->type == NODE_VAR && strcmp(node->str_val, "NULL") == 0;
}

static int is_class_subtype(Symbol *derived, Symbol *base) {
    while (derived) {
        if (derived == base) return 1;
        if (derived->base_class && derived->inheritance_modifier != 0) return 0;
        derived = derived->base_class;
    }
    return 0;
}

/* Check if two types are compatible, including implicit void* conversions.
 * Returns 1 if types are compatible, 0 otherwise. */
static int types_compatible(DataType t1, int p1, Symbol *s1,
                            DataType t2, int p2, Symbol *s2) {
    /* Exact match */
    if (t1 == t2 && p1 == p2 && (t1 != TYPE_STRUCT || s1 == s2)) return 1;

    /* One is void pointer, other is any pointer type with same indirection level */
    if (p1 > 0 && p1 == p2) {
        if (is_void_pointer(t1, p1) || is_void_pointer(t2, p2)) return 1;
    }

    /* Class pointer conversions: Derived* -> Base* */
    if (t1 == TYPE_STRUCT && t2 == TYPE_STRUCT && p1 > 0 && p2 > 0 && p1 == p2 && s1 && s2) {
        if (is_class_subtype(s1, s2)) return 1;
    }

    if (t1 == TYPE_STRUCT && t2 == TYPE_STRUCT && p1 == p2 && s1 != s2) {
        // Log mismatch for debugging
    }

    /* Both are non-void pointers to same base type and same indirection count */
    if (p1 > 0 && p2 > 0 && t1 == t2 && p1 == p2 && t1 != TYPE_STRUCT) return 1;

    return 0;
}

/* Initialize built-in functions (malloc, free) in the global symbol table */
static void init_builtin_functions() {
    /* malloc: void* malloc(int) */
    Symbol *malloc_sym = create_symbol("malloc", TYPE_VOID, SYM_FUNCTION, 0);
    malloc_sym->pointer_level = 1;  /* Return type is void* */
    malloc_sym->param_count = 1;
    malloc_sym->param_types = malloc(sizeof(DataType) * 1);
    malloc_sym->param_types[0] = TYPE_INT;
    malloc_sym->param_is_array = malloc(sizeof(int) * 1);
    malloc_sym->param_is_array[0] = 0;
    malloc_sym->param_names = malloc(sizeof(char*) * 1);
    malloc_sym->param_names[0] = strdup("size");
    insert_symbol(malloc_sym);
    
    /* free: void free(void*) */
    Symbol *free_sym = create_symbol("free", TYPE_VOID, SYM_FUNCTION, 0);
    free_sym->pointer_level = 0;  /* Return type is void */
    free_sym->param_count = 1;
    free_sym->param_types = malloc(sizeof(DataType) * 1);
    free_sym->param_types[0] = TYPE_VOID;
    free_sym->param_is_array = malloc(sizeof(int) * 1);
    free_sym->param_is_array[0] = 0;
    free_sym->param_names = malloc(sizeof(char*) * 1);
    free_sym->param_names[0] = strdup("ptr");
    insert_symbol(free_sym);

    /* NULL: void* constant pointer */
    Symbol *null_sym = create_symbol("NULL", TYPE_VOID, SYM_CONSTANT, 0);
    null_sym->pointer_level = 1;
    null_sym->is_const = 1;
    null_sym->const_value = 0;
    null_sym->has_const_value = 1;
    insert_symbol(null_sym);
}

const char* type_to_string(DataType t) {
    switch (t) {
        case TYPE_INT: return "int";
        case TYPE_CHAR: return "char";
        case TYPE_VOID: return "void";
        case TYPE_STRUCT: return "struct";
        default: return "unknown";
    }
}

char* get_mangled_name(const char *prefix, const char *name, ASTNode *params) {
    char buf[2048];  // Increased buffer size
    int pos = 0;
    
    // Initial name construction
    if (prefix)
        pos = snprintf(buf, sizeof(buf), "%s_%s", prefix, name);
    else
        pos = snprintf(buf, sizeof(buf), "%s", name);
    
    // Append parameter types safely
    ASTNode *p = params;
    while (p && pos < 2000) {  // Leave margin for safety
        if (p->str_val && strcmp(p->str_val, "this") == 0) {
            p = p->next;
            continue;
        }
        int n = snprintf(&buf[pos], sizeof(buf) - pos, "_%s", 
                        type_to_string(p->left->data_type));
        if (n >= 0 && n < (int)(sizeof(buf) - pos)) {
            pos += n;
        } else {
            break;  // Stop if buffer would overflow
        }
        p = p->next;
    }
    
    return strdup(buf);
}

void analyze_struct_def(ASTNode *node) {
    if (!node || node->type != NODE_STRUCT_DEF) return;

    if (!node->str_val) {
        semantic_error(node->line_number, "Unnamed struct definitions are not supported");
        return;
    }

    Symbol *existing = lookup(node->str_val);
    if (existing) {
        semantic_error(node->line_number, "Struct redeclared");
        return;
    }

    Symbol *sym = create_symbol(node->str_val, TYPE_STRUCT, SYM_STRUCT, node->line_number);
    if (!insert_symbol(sym)) {
        semantic_error(node->line_number, "Failed to insert struct symbol");
        return;
    }

    sym->is_class = node->is_class;
    current_class = sym;
    int current_access = node->is_class ? 1 : 0; 
    int offset = 0;
    int has_base_vtable = 0;

    if (node->base_class_name) {
        Symbol *base = lookup(node->base_class_name);
        if (!base || base->kind != SYM_STRUCT) {
            semantic_error(node->line_number, "Unknown base class");
            return;
        }
        sym->base_class = base;
        sym->inheritance_modifier = node->inheritance_modifier;

        Symbol *b_mem = base->members;
        while (b_mem) {
            Symbol *m = create_symbol(b_mem->name, b_mem->type, b_mem->kind, b_mem->line_number);
            if (b_mem->unmangled_name) m->unmangled_name = strdup(b_mem->unmangled_name);
            m->pointer_level = b_mem->pointer_level;
            m->struct_def = b_mem->struct_def;
            m->is_array = b_mem->is_array;
            m->array_size = b_mem->array_size;
            if (node->inheritance_modifier == 1) m->access_modifier = 1;
            else m->access_modifier = b_mem->access_modifier;
            m->defining_struct = b_mem->defining_struct;
            m->struct_offset = b_mem->struct_offset;

            /* Full copy for functions (parameters) */
            if (b_mem->kind == SYM_FUNCTION) {
                m->param_count = b_mem->param_count;
                if (m->param_count > 0) {
                    m->param_types = malloc(sizeof(DataType) * m->param_count);
                    memcpy(m->param_types, b_mem->param_types, sizeof(DataType) * m->param_count);
                    m->param_pointer_levels = malloc(sizeof(int) * m->param_count);
                    memcpy(m->param_pointer_levels, b_mem->param_pointer_levels, sizeof(int) * m->param_count);
                    m->param_struct_defs = malloc(sizeof(Symbol*) * m->param_count);
                    memcpy(m->param_struct_defs, b_mem->param_struct_defs, sizeof(Symbol*) * m->param_count);
                    m->param_is_array = malloc(sizeof(int) * m->param_count);
                    memcpy(m->param_is_array, b_mem->param_is_array, sizeof(int) * m->param_count);
                    m->param_names = malloc(sizeof(char*) * m->param_count);
                    for (int i = 0; i < m->param_count; i++) m->param_names[i] = strdup(b_mem->param_names[i]);
                }
            }

            m->next_member = sym->members;
            sym->members = m;
            b_mem = b_mem->next_member;
        }

        Symbol *b_v = base->virtual_methods;
        while (b_v) {
            /* Find the already copied member in 'sym' that corresponds to this virtual method */
            Symbol *v = find_struct_member(sym, b_v->name);
            if (v) {
                v->is_virtual = 1;
                v->vtable_index = b_v->vtable_index;
                v->next_virtual = sym->virtual_methods;
                sym->virtual_methods = v;
            }
            b_v = b_v->next_virtual;
        }

        offset = base->struct_size;
        if (base->virtual_methods) has_base_vtable = 1;
    }

    for (ASTNode *member = node->body; member; member = member->next) {
        if (!member) continue;

        if (member->type == NODE_ACCESS_SPEC) {
            current_access = member->access_modifier;
            continue;
        }

        if (member->type == NODE_FUNC_DEF) {
            char mangled_name[256];
            if (member->is_constructor) {
                snprintf(mangled_name, sizeof(mangled_name), "%s__ctor", sym->name);
            } else if (member->is_destructor) {
                snprintf(mangled_name, sizeof(mangled_name), "%s__dtor", sym->name);
            } else {
                char *new_mangled = get_mangled_name(sym->name, member->str_val, member->params);
                strncpy(mangled_name, new_mangled, sizeof(mangled_name));
                free(new_mangled);
            }
            char *orig_name = member->str_val;
            member->str_val = strdup(mangled_name);

            int has_this = 0;
            for (ASTNode *p = member->params; p; p = p->next) {
                if (p->str_val && strcmp(p->str_val, "this") == 0) {
                    has_this = 1;
                    break;
                }
            }
            if (!has_this) {
                ASTNode *this_type = create_node(NODE_TYPE);
                this_type->data_type = TYPE_STRUCT;
                this_type->str_val = strdup(sym->name);
                this_type->pointer_level = 1;
                
                ASTNode *this_param = create_node(NODE_PARAM);
                this_param->str_val = strdup("this");
                this_param->left = this_type;
                /* Pointer level is carried by this_type->pointer_level. */
                this_param->pointer_level = 0;
                this_param->line_number = member->line_number;
                
                this_param->next = member->params;
                member->params = this_param;
            }

            analyze_function(member, orig_name);
            Symbol *func = lookup(member->str_val);

            if (func) {
                func->unmangled_name = orig_name; 
                func->access_modifier = current_access;
                func->defining_struct = sym;
                Symbol *existing_v = find_virtual_method(sym, orig_name);
                if (member->is_virtual || existing_v) {
                    func->is_virtual = 1;
                    if (existing_v) {
                        func->vtable_index = existing_v->vtable_index;
                        replace_virtual_method(sym, func);
                    } else {
                        func->next_virtual = sym->virtual_methods;
                        sym->virtual_methods = func;
                    }
                }
            }
            continue;
        }

        DataType member_type = TYPE_VOID;
        int member_pointer = 0;
        Symbol *member_struct_def = NULL;
        resolve_decl_type(member->left, member->pointer_level, member->line_number,
                  &member_type, &member_pointer, &member_struct_def);

        Symbol *m = create_symbol(member->str_val, member_type, SYM_VARIABLE, member->line_number);
        m->access_modifier = current_access;
        m->defining_struct = sym;
        m->pointer_level = member_pointer;
        m->struct_def = member_struct_def;
        m->array_dim_count = member->array_dim_count;
        if (member->array_dim_count > 0) {
            m->is_array = 1;
            m->array_sizes = malloc(sizeof(int) * member->array_dim_count);
            for (int i = 0; i < member->array_dim_count; i++) {
                ASTNode *expr = member->array_dim_exprs ? member->array_dim_exprs[i] : NULL;
                if (expr && expr->type == NODE_CONST_INT) {
                    m->array_sizes[i] = expr->int_val;
                } else if (expr && expr->type == NODE_VAR) {
                    /* Try to resolve constant variable (e.g. int MAX = 100) */
                    Symbol *dim_sym = lookup_all_scopes(expr->str_val);
                    if (dim_sym && dim_sym->const_value > 0) {
                        m->array_sizes[i] = dim_sym->const_value;
                    } else {
                        m->array_sizes[i] = -1;
                    }
                } else {
                    m->array_sizes[i] = -1;
                }
            }
        }

        int size = get_type_size(m->type, m->pointer_level, m->struct_def);
        if (m->array_dim_count > 0) {
            int total = size;
            for (int i = 0; i < m->array_dim_count; i++) {
                if (m->array_sizes[i] <= 0) { total = 0; break; }
                total *= m->array_sizes[i];
            }
            size = total;
        }

        /* Align member offset based on type size (1, 4, or 8 bytes) */
        int align = (m->pointer_level > 0) ? 8 : 4;
        if (m->type == TYPE_CHAR && m->pointer_level == 0) align = 1;
        if (m->type == TYPE_STRUCT && m->pointer_level == 0) align = 8; /* Conservative alignment for structs */
        
        offset = (offset + align - 1) & ~(align - 1);

        m->struct_offset = offset;
        offset += size;
        m->next_member = sym->members;
        sym->members = m;
    }

    sym->struct_size = offset;

    if (sym->virtual_methods && !has_base_vtable) {
        int ptr_size = 8;
        Symbol *m = sym->members;
        while (m) {
            m->struct_offset += ptr_size;
            m = m->next_member;
        }
        sym->struct_size += ptr_size;

        int idx = 0;
        Symbol *v = sym->virtual_methods;
        while (v) {
            v->vtable_index = idx++;
            v = v->next_virtual;
        }
        sym->vtable_size = idx;
    } else if (sym->virtual_methods && has_base_vtable) {
        int idx = 0;
        Symbol *b_v_search = sym->base_class->virtual_methods;
        while (b_v_search) {
            if (b_v_search->vtable_index >= idx) idx = b_v_search->vtable_index + 1;
            b_v_search = b_v_search->next_virtual;
        }
        
        Symbol *v = sym->virtual_methods;
        while (v) {
            if (v->vtable_index == -1) {
                v->vtable_index = idx++;
            }
            v = v->next_virtual;
        }
        sym->vtable_size = idx;
    }
    
    current_class = NULL;

}

void analyze_function(ASTNode *node, const char *unmangled_name) {
    DataType ret_type = TYPE_VOID;
    int ret_pointer = 0;
    Symbol *ret_struct_def = NULL;
    resolve_decl_type(node->left, 0, node->line_number, &ret_type, &ret_pointer, &ret_struct_def);

    Symbol *func = create_symbol(
        node->str_val,
        ret_type,
        SYM_FUNCTION,
        node->line_number
    );
    func->pointer_level = ret_pointer;
    func->struct_def = ret_struct_def;

    if (current_class) {
        func->defining_struct = current_class;
        func->next_member = current_class->members;
        current_class->members = func;
    }

    if (!insert_symbol(func)) {
        semantic_error(node->line_number, "Function redeclared");
        return;
    }

    current_function = func;
    if (unmangled_name) {
        func->unmangled_name = strdup(unmangled_name);
    }
    int count = 0;
    ASTNode *param = node->params;

    while (param) {
        count++;
        param = param->next;
    }

    func->param_count = count;

    if (count > 0) {
        func->param_types = malloc(sizeof(DataType) * count);
        func->param_pointer_levels = malloc(sizeof(int) * count);
        func->param_struct_defs = malloc(sizeof(Symbol*) * count);
        func->param_is_array = malloc(sizeof(int) * count);
        func->param_names = malloc(sizeof(char*) * count);
    }

    param = node->params;
    int i = 0;
    while (param) {
        DataType p_type = TYPE_VOID;
        int p_ptr = 0;
        Symbol *p_struct_def = NULL;
        resolve_decl_type(param->left, param->pointer_level, param->line_number, &p_type, &p_ptr, &p_struct_def);
        func->param_types[i] = p_type;
        func->param_pointer_levels[i] = p_ptr;
        func->param_struct_defs[i] = p_struct_def;
        func->param_is_array[i] = (param->int_val != 0);
        func->param_names[i] = strdup(param->str_val);
        param = param->next;
        i++;
    }

    enter_scope();
    func->scope = current_scope;
    param = node->params;
    i = 0;
    current_local_offset = 0;
    while (param) {
        DataType p_type = TYPE_VOID;
        int p_ptr = 0;
        Symbol *p_struct_def = NULL;
        resolve_decl_type(param->left, param->pointer_level, param->line_number, &p_type, &p_ptr, &p_struct_def);

        Symbol *sym = create_symbol(
            param->str_val,
            p_type,
            SYM_PARAMETER,
            param->line_number
        );

        sym->pointer_level = p_ptr;
        sym->struct_def = p_struct_def;

        if (param->int_val != 0) {
            sym->is_array = 1;
            sym->array_size = -1; 
        }

        current_local_offset += 8;
        sym->frame_offset = -current_local_offset;

        if (!insert_symbol(sym))
            semantic_error(param->line_number, "Parameter redeclared");

        param = param->next;
        i++;
    }

    int body_returns = analyze_node(node->body);
    current_function->local_vars_size = current_local_offset;

     if (current_function->type != TYPE_VOID && !body_returns) {
        semantic_error(node->line_number, "Non-void function must return a value");
    }

    exit_scope();
    current_function = NULL;
}

void analyze_declaration(ASTNode *node) {
    DataType decl_type = TYPE_VOID;
    int decl_pointer = 0;
    Symbol *decl_struct_def = NULL;
    resolve_decl_type(node->left, node->pointer_level, node->line_number,
                      &decl_type, &decl_pointer, &decl_struct_def);

    if (node->is_typedef) {
        if (node->right) {
            semantic_error(node->line_number, "typedef alias cannot have an initializer");
        }
        if (node->array_dim_count > 0) {
            semantic_error(node->line_number, "typedef arrays are not supported in this compiler yet");
        }

        Symbol *tsym = create_symbol(node->str_val, decl_type, SYM_TYPEDEF, node->line_number);
        tsym->pointer_level = decl_pointer;
        tsym->struct_def = decl_struct_def;
        tsym->is_const = node->is_const;

        if (!insert_symbol(tsym)) {
            semantic_error(node->line_number, "Type alias redeclared");
        }
        return;
    }

    Symbol *sym = create_symbol(
        node->str_val,
        decl_type,
        SYM_VARIABLE,
        node->line_number
    );

    if (node->type == NODE_ARRAY_DECL) {
        if (node->int_val <= 0) semantic_error(node->line_number, "Array size must be positive");
        else if (decl_type == TYPE_VOID) semantic_error(node->line_number, "Array element type cannot be void");
        sym->is_array = 1;
        sym->array_size = node->int_val;
    } else if (node->type == NODE_VAR_DECL) {
        sym->pointer_level = decl_pointer;
        sym->struct_def = decl_struct_def;
        sym->array_dim_count = node->array_dim_count;
        if (node->array_dim_count > 0) {
            sym->is_array = 1;
            sym->array_sizes = malloc(sizeof(int) * node->array_dim_count);
            sym->array_dim_exprs = malloc(sizeof(ASTNode*) * node->array_dim_count);
            for (int i = 0; i < node->array_dim_count; i++) {
                ASTNode *expr = node->array_dim_exprs[i];
                if (expr && expr->type == NODE_CONST_INT) {
                    sym->array_sizes[i] = expr->int_val;
                } else {
                    sym->array_sizes[i] = -1;
                    sym->array_dim_exprs[i] = expr;
                    sym->is_vla = 1;
                }
            }
        }
    }

    sym->pointer_level = decl_pointer;
    sym->struct_def = decl_struct_def;

    int size = get_type_size(decl_type, sym->pointer_level, sym->struct_def);
    if (sym->is_array && sym->array_size > 0) {
        size = size * sym->array_size;
    } else if (sym->is_array && sym->array_dim_count > 0) {
        int total_elements = 1;
        for (int i=0; i < sym->array_dim_count; i++) {
            if (sym->array_sizes[i] > 0) total_elements *= sym->array_sizes[i];
        }
        size = size * total_elements;
    } else if (sym->pointer_level > 0 || sym->is_array) {
        size = 8;
    }

    size = (size + 3) & ~3;
    current_local_offset += size;
    sym->frame_offset = -current_local_offset;

    sym->is_const = node->is_const;

    if (!insert_symbol(sym))
        semantic_error(node->line_number, "Variable redeclared");
    
    node->sym = sym;

    if (node->right) {
        analyze_node(node->right);
        if (!types_compatible(node->right->data_type, node->right->pointer_level, node->right->struct_def,
                              sym->type, sym->pointer_level, sym->struct_def)) {
            semantic_error(node->line_number, "Type mismatch in initialization");
        }
        
        if (node->right->type == NODE_CONST_INT && sym->is_const) {
            /* Store constant value for array size resolution ONLY if actually 'const' */
            sym->has_const_value = 1;
            sym->const_value = node->right->int_val;
        }
    }
}

int analyze_block(ASTNode *node) {
    enter_scope();
    ASTNode *stmt = node->left;
    int returns = 0;
    while (stmt) {
        returns = analyze_node(stmt);
        if (returns) break;
        stmt = stmt->next;
    }
    exit_scope();
    return returns;
}

// Handle Implicit 'this->' for Object-Oriented Class Members
void analyze_variable(ASTNode *node) {
    Symbol *sym = lookup(node->str_val);
    
    if (!sym) {
        /* If the variable isn't in local scope, check if we are inside a class method */
        Symbol *class_sym = NULL;
        if (current_function && current_function->defining_struct) {
            class_sym = current_function->defining_struct;
        } else if (current_class) {
            class_sym = current_class;
        }
        if (class_sym) {
            Symbol *member = find_struct_member(class_sym, node->str_val);
            if (member) {
                /* Implicit 'this->' resolution! 
                   Transform this NODE_VAR into a NODE_MEMBER_ACCESS (this->var) */
                node->type = NODE_MEMBER_ACCESS;
                node->int_val = 1; /* 1 means arrow access '->' */
                
                /* Create the implicit 'this' pointer node */
                ASTNode *this_node = create_node(NODE_VAR);
                this_node->str_val = strdup("this");
                this_node->data_type = TYPE_STRUCT;
                this_node->pointer_level = 1;
                this_node->struct_def = class_sym;
                this_node->line_number = node->line_number;
                
                node->left = this_node;
                analyze_node(this_node); /* Link 'this' symbol */
                
                /* Copy the member's properties */
                node->data_type = member->type;
                node->pointer_level = member->pointer_level;
                node->struct_def = member->struct_def;
                node->member_sym = member;
                node->member_offset = member->struct_offset;
                return;
            } else {
                fprintf(stderr, "DEBUG: failed implicit member lookup '%s' in class '%s'\n", node->str_val, class_sym->name);
            }
        }
        
        semantic_error_undeclared_with_hint(node->line_number, "variable", node->str_val, (SymbolKind)-1);
        node->data_type = TYPE_INT;
        return;
    }
    
    /* Standard local/global variable resolution */
    node->sym = sym;
    node->data_type = sym->type;
    node->pointer_level = sym->pointer_level; 
    node->struct_def = sym->struct_def;       
}

void analyze_member_access(ASTNode *node) {
    analyze_node(node->left);
    if (!node->left) {
        semantic_error(node->line_number, "Invalid member access");
        node->data_type = TYPE_INT;
        return;
    }
    if (node->left->type != NODE_VAR && node->left->type != NODE_MEMBER_ACCESS &&
        node->left->type != NODE_INDEX && node->left->type != NODE_FUNC_CALL) {
        semantic_error(node->line_number, "Unsupported member access base expression");
        node->data_type = TYPE_INT;
        return;
    }

    Symbol *struct_def = node->left->struct_def;
    if (!struct_def && node->left->type == NODE_VAR) {
        Symbol *base = lookup_all_scopes(node->left->str_val);
        if (base) struct_def = base->struct_def;
    }

    if (node->int_val == 1) { // arrow
        /* For arrows, we might need to check if it's a pointer, 
           but struct_def should already be set from analyze_node(node->left) */
    }

    if (!struct_def) {
        semantic_error(node->line_number, "Unknown struct type in member access");
        node->data_type = TYPE_INT;
        return;
    }
    node->struct_def = struct_def;

    Symbol *member = find_struct_member(struct_def, node->str_val);
    if (!member) {
        // Check if it's a member function
        char mangled[256];
        snprintf(mangled, sizeof(mangled), "%s_%s", struct_def->name, node->str_val);
        Symbol *func = lookup(mangled);
        if (func && func->kind == SYM_FUNCTION) {
            // Do NOT transform here; let analyze_function_call handle it to correctly identify member calls
            node->data_type = func->type;
            node->pointer_level = func->pointer_level;
            node->struct_def = func->struct_def;
            return;
        }
        semantic_error(node->line_number, "Struct member not found");
        node->data_type = TYPE_INT;
        return;
    }

    if (member->access_modifier == 1) { 
        if (!current_class || current_class != member->defining_struct) {
            semantic_error(node->line_number, "Private member access violation");
        }
    }
    node->data_type = member->type;
    node->pointer_level = member->pointer_level;
    node->member_sym = member;
    node->member_offset = member->struct_offset;
    node->struct_def = member->struct_def; // The type of the member itself
}

void analyze_assignment(ASTNode *node) {
    analyze_node(node->left);
    analyze_node(node->right);
    if (node->left->data_type == TYPE_VOID || node->right->data_type == TYPE_VOID) return;

    if (node->left->type != NODE_VAR && node->left->type != NODE_INDEX &&
        node->left->type != NODE_MEMBER_ACCESS &&
        !(node->left->type == NODE_UN_OP && node->left->int_val == '*')) {
        semantic_error(node->line_number, "Left-hand side of assignment must be a modifiable lvalue");
        return;
    }

    if (node->left->sym && node->left->sym->is_const) {
        semantic_error(node->line_number, "Assignment to const variable");
    }

    if (!types_compatible(node->right->data_type, node->right->pointer_level, node->right->struct_def,
                          node->left->data_type, node->left->pointer_level, node->left->struct_def)) {
        semantic_error(node->line_number, "Assignment type mismatch");
    }
    node->data_type = node->left->data_type;
}

void analyze_binary(ASTNode *node) {
    analyze_node(node->left);
    analyze_node(node->right);

    /* Check if this is a comparison operator */
    int is_comparison = (node->int_val == T_EQ || node->int_val == T_NEQ ||
                        node->int_val == '<' || node->int_val == '>' ||
                        node->int_val == T_LE || node->int_val == T_GE);

    if (is_comparison) {
        /* For comparisons, result is always int */
        node->data_type = TYPE_INT;
        node->pointer_level = 0;

        /* Allow comparisons between compatible types (symmetric) */
        if (types_compatible(node->left->data_type, node->left->pointer_level, node->left->struct_def,
                             node->right->data_type, node->right->pointer_level, node->right->struct_def) ||
            types_compatible(node->right->data_type, node->right->pointer_level, node->right->struct_def,
                             node->left->data_type, node->left->pointer_level, node->left->struct_def)) {
            return;
        }

        /* Allow pointer compared to NULL (void*) */
        if ((node->left->pointer_level > 0 && (is_null_constant(node->right) || is_void_pointer(node->right->data_type, node->right->pointer_level))) ||
            (node->right->pointer_level > 0 && (is_null_constant(node->left) || is_void_pointer(node->left->data_type, node->left->pointer_level)))) {
            return;
        }

        semantic_error(node->line_number, "Comparison operand type mismatch");
        return;
    }

    if (node->left->data_type == TYPE_VOID || node->right->data_type == TYPE_VOID) return;

    /* For arithmetic operators, require exact type match */
    if (node->left->data_type == node->right->data_type &&
        node->left->pointer_level == node->right->pointer_level) {
        node->data_type = node->left->data_type;
        node->pointer_level = node->left->pointer_level;
        node->struct_def = node->left->struct_def;
        return;
    }

    /* Pointer arithmetic: pointer +/- int */
    if ((node->int_val == '+' || node->int_val == '-') &&
        ((node->left->pointer_level > 0 && node->right->data_type == TYPE_INT && node->right->pointer_level == 0) ||
         (node->right->pointer_level > 0 && node->left->data_type == TYPE_INT && node->left->pointer_level == 0))) {
        if (node->left->pointer_level > 0) {
            node->data_type = node->left->data_type;
            node->pointer_level = node->left->pointer_level;
            node->struct_def = node->left->struct_def;
        } else {
            node->data_type = node->right->data_type;
            node->pointer_level = node->right->pointer_level;
            node->struct_def = node->right->struct_def;
        }
        return;
    }

    semantic_error(node->line_number, "Binary operand type mismatch");
    node->data_type = node->left->data_type;
    node->pointer_level = node->left->pointer_level;
}

void analyze_unary(ASTNode *node) {
    analyze_node(node->left);
    if (node->left) {
        if (node->int_val == '&' && node->left->type == NODE_VAR) {
            Symbol *sym = lookup(node->left->str_val);
            if (sym) sym->is_address_taken = 1;
            node->pointer_level = node->left->pointer_level + 1;
        } else if (node->int_val == '*') {
            if (node->left->pointer_level > 0) {
                node->pointer_level = node->left->pointer_level - 1;
            } else {
                semantic_error(node->line_number, "Cannot dereference non-pointer type");
                node->pointer_level = 0;
            }
        } else {
            node->pointer_level = node->left->pointer_level;
        }
        node->data_type = node->left->data_type;
        node->struct_def = node->left->struct_def;
    } else {
        node->data_type = TYPE_INT;
        node->pointer_level = 0;
        node->struct_def = NULL;
    }
}

void analyze_increment_decrement(ASTNode *node) {
    analyze_node(node->left);
    if (!node->left) {
        semantic_error(node->line_number, "Invalid operand for increment/decrement");
        node->data_type = TYPE_INT;
        return;
    }
    
    /* Operand must be a modifiable lvalue */
    if (node->left->type != NODE_VAR && node->left->type != NODE_INDEX &&
        node->left->type != NODE_MEMBER_ACCESS &&
        !(node->left->type == NODE_UN_OP && node->left->int_val == '*')) {
        semantic_error(node->line_number, "Operand of increment/decrement must be a modifiable lvalue");
    }

    if (node->left->sym && node->left->sym->is_const) {
        semantic_error(node->line_number, "Increment/decrement of const variable");
    }

    if (node->left->data_type != TYPE_INT && node->left->data_type != TYPE_CHAR && node->left->pointer_level == 0) {
        semantic_error(node->line_number, "Increment/decrement requires arithmetic or pointer type");
    }

    node->data_type = node->left->data_type;
    node->pointer_level = node->left->pointer_level;
}

void analyze_index(ASTNode *node) {
    if (!node || node->type != NODE_INDEX) return;
    ASTNode *base = node->left;
    ASTNode *idx = node->right;
    analyze_node(base);
    analyze_node(idx);
    if (idx->data_type != TYPE_INT) semantic_error(node->line_number, "Array index expression must be of type int");

    if (base->type == NODE_VAR) {
        Symbol *sym = lookup(base->str_val);
        if (!sym) {
            semantic_error(node->line_number, "Undeclared variable in indexing expression");
            node->data_type = TYPE_INT;
            return;
        }
        if (sym->pointer_level > 0) {
            node->data_type = sym->type;
        } else if (sym->is_array && sym->array_dim_count > 0) {
            node->data_type = sym->type;
        } else {
            semantic_error(node->line_number, "Indexing is only allowed on array variables or pointers");
            node->data_type = sym->type;
            return;
        }
    } else if (base->type == NODE_INDEX) {
        node->data_type = base->data_type;
    } else if (base->type == NODE_MEMBER_ACCESS) {
        /* Support indexing on struct members like this->arr[i] */
        Symbol *m_sym = base->member_sym;
        if (m_sym && (m_sym->pointer_level > 0 || m_sym->is_array)) {
            node->data_type = m_sym->type;
            node->pointer_level = m_sym->pointer_level > 0 ? m_sym->pointer_level - 1 : 0;
            node->struct_def = m_sym->struct_def;
        } else {
            semantic_error(node->line_number, "Member used for indexing must be a pointer or array");
            node->data_type = TYPE_INT;
        }
    } else {
        semantic_error(node->line_number, "Invalid base expression for array indexing");
        node->data_type = TYPE_INT;
    }
}

// CRITICAL FIX: Clones the 'this' argument so it doesn't accidentally
// mutate the AST DAG structure and cause duplicate statements.
void analyze_function_call(ASTNode *node) {
    if (node->left && node->left->type == NODE_VAR) {
        Symbol *callee = lookup(node->left->str_val);
        if (callee) {
            node->left->sym = callee;
            node->left->data_type = callee->type;
            node->left->pointer_level = callee->pointer_level;
            node->left->struct_def = callee->struct_def;
        } else if (current_function && current_function->defining_struct) {
            /* Unqualified call inside a class method: resolve as implicit this->method(...) */
            Symbol *cls = current_function->defining_struct;
            Symbol *m = find_struct_member(cls, node->left->str_val);
            if (m && m->kind == SYM_FUNCTION) {
                ASTNode *this_node = create_node(NODE_VAR);
                this_node->str_val = strdup("this");
                this_node->line_number = node->line_number;

                ASTNode *member = create_node(NODE_MEMBER_ACCESS);
                member->line_number = node->line_number;
                member->left = this_node;
                member->str_val = strdup(node->left->str_val);
                member->int_val = 1; /* arrow */

                node->left = member;
                analyze_node(node->left);
            }
        }
    } else {
        analyze_node(node->left);
    }

    ASTNode *arg = node->right;
    while (arg) {
        analyze_node(arg);
        arg = arg->next;
    }

    Symbol *sym = NULL;

    if (node->left->type == NODE_VAR) {
        sym = lookup(node->left->str_val);
    } else if (node->left->type == NODE_MEMBER_ACCESS) {
        /* The base object's struct is in node->left->left->struct_def.
         * node->left->struct_def may be the return type's struct (wrong for void methods). */
        ASTNode *base_obj = node->left->left;
        Symbol *obj_struct = base_obj ? base_obj->struct_def : NULL;
        /* For pointer bases, lookup_all_scopes to get the struct */
        if (!obj_struct && base_obj && base_obj->type == NODE_VAR) {
            Symbol *vsym = lookup_all_scopes(base_obj->str_val);
            if (vsym) obj_struct = vsym->struct_def;
        }
        if (obj_struct) {
            char buf[2048];  // Increased size
            int pos = snprintf(buf, sizeof(buf), "%s_%s", obj_struct->name, node->left->str_val);

            arg = node->right;
            while (arg && pos < 2000) {  // Safety margin
                int n = snprintf(&buf[pos], sizeof(buf) - pos, "_%s",
                                type_to_string(arg->data_type));
                if (n >= 0 && n < (int)(sizeof(buf) - pos)) {
                    pos += n;
                } else {
                    break;  // Stop if would overflow
                }
                arg = arg->next;
            }
            sym = lookup(buf);
            if (!sym) {
                Symbol *c = obj_struct;
                while (c && !sym) {
                    char nested_buf[2048];
                    int nested_pos = snprintf(nested_buf, sizeof(nested_buf), "%s_%s", c->name, node->left->str_val);
                    arg = node->right;
                    while (arg && nested_pos < (int)sizeof(nested_buf) - 1) {
                        int n = snprintf(&nested_buf[nested_pos], sizeof(nested_buf) - nested_pos, "_%s",
                                         type_to_string(arg->data_type));
                        if (n >= 0 && n < (int)(sizeof(nested_buf) - nested_pos)) {
                            nested_pos += n;
                        } else {
                            break;
                        }
                        arg = arg->next;
                    }
                    sym = lookup(nested_buf);
                    if (!sym) {
                        char fallback[256];
                        snprintf(fallback, sizeof(fallback), "%s_%s", c->name, node->left->str_val);
                        sym = lookup(fallback);
                    }
                    c = c->base_class;
                }
            }
            if (!sym) {
                /* Search virtual methods in obj_struct and its bases */
                Symbol *c = obj_struct;
                while (c && !sym) {
                    Symbol *v = c->virtual_methods;
                    while (v) {
                        if (v->unmangled_name && strcmp(v->unmangled_name, node->left->str_val) == 0) {
                            sym = lookup(v->name);
                            break;
                        }
                        v = v->next_virtual;
                    }
                    c = c->base_class;
                }
            }
            /* Ensure node->left->struct_def is the object's struct for later use */
            node->left->struct_def = obj_struct;
        }
        
        if (sym) {
            if (sym->is_virtual) node->is_virtual_call = 1;
            node->call_struct = node->left->struct_def;
            
            // Safely create a clone of the base object to pass as the 'this' parameter
            ASTNode *obj_expr = node->left->left;
            ASTNode *this_arg = create_node(NODE_VAR);
            this_arg->str_val = strdup(obj_expr->str_val);
            this_arg->data_type = obj_expr->data_type;
            this_arg->pointer_level = obj_expr->pointer_level;
            this_arg->struct_def = obj_expr->struct_def;
            this_arg->line_number = obj_expr->line_number;
            this_arg->sym = obj_expr->sym; 

            if (this_arg->data_type == TYPE_STRUCT && this_arg->pointer_level == 0) {
                ASTNode *addr_node = create_node(NODE_UN_OP);
                addr_node->int_val = '&';
                addr_node->left = this_arg;
                addr_node->data_type = TYPE_STRUCT;
                addr_node->pointer_level = 1;
                addr_node->struct_def = this_arg->struct_def;
                addr_node->line_number = this_arg->line_number;
                addr_node->sym = obj_expr->sym;
                this_arg = addr_node;
            }
            this_arg->next = node->right;
            node->right = this_arg;
            analyze_node(this_arg); // Now analyze it to ensure it's fully linked!
        }
    } else {
        semantic_error(node->line_number, "Invalid function call expression");
        return;
    }

    if (!sym || sym->kind != SYM_FUNCTION) {
        const char *name = (node->left && node->left->type == NODE_VAR) ? node->left->str_val :
                           (node->left && node->left->type == NODE_MEMBER_ACCESS ? node->left->str_val : "<call>");
        semantic_error_undeclared_with_hint(node->line_number, "function", name, SYM_FUNCTION);
        return;
    }

    node->func_sym = sym;
    arg = node->right;
    int i = 0;
    int is_member_call = (node->left->type == NODE_MEMBER_ACCESS);

    /* For member calls, check the implicit 'this' parameter */
    if (is_member_call) {
        if (!arg) {
            semantic_error(node->line_number, "Missing 'this' argument for member call");
            return;
        }
        /* 'this' should be a pointer to the defining struct */
        if (!types_compatible(arg->data_type, arg->pointer_level, arg->struct_def,
                              TYPE_STRUCT, 1, sym->defining_struct)) {
            semantic_error(node->line_number, "Invalid 'this' argument type");
        }
        arg = arg->next;
        i = 0;  /* Start checking explicit args at param index 0 */
        /* Now arg points to the first explicit argument */
    }

    while (arg) {
        if (i >= (is_member_call ? sym->param_count - 1 : sym->param_count)) {
            semantic_error(node->line_number, "Too many arguments");
            break;
        }
        
        /* Type compatibility check: for member calls, account for the 'this' pointer in param_types[0] */
        int param_idx = is_member_call ? i + 1 : i;

        int param_pointer_level = (sym->param_pointer_levels ? sym->param_pointer_levels[param_idx] : 0);
        Symbol *param_struct_def = NULL;
        if (sym->param_struct_defs) {
            param_struct_def = sym->param_struct_defs[param_idx];
        }
        
        if (sym->param_types[param_idx] == TYPE_VOID && param_pointer_level == 0) {
            /* For parameters, we assume void* if type is void and function likely uses it as pointer */
            if (strcmp(sym->name, "free") == 0) {
                param_pointer_level = 1;
            }
        }
        
        if (!types_compatible(arg->data_type, arg->pointer_level, arg->struct_def,
                              sym->param_types[param_idx], param_pointer_level, param_struct_def)) {
            if (!(param_idx == 0 && sym->param_types[0] == TYPE_STRUCT && arg->data_type == TYPE_STRUCT)) {
                semantic_error(node->line_number, "Argument type mismatch");
            }
        }
        arg = arg->next;
        i++;
    }

    if (i < (is_member_call ? sym->param_count - 1 : sym->param_count)) semantic_error(node->line_number, "Too few arguments");
    node->data_type = sym->type;
    node->pointer_level = sym->pointer_level;  /* Set return type pointer level */
    node->struct_def = sym->struct_def;        /* Set return type struct definition */
}

int analyze_return(ASTNode *node) {
   if (!current_function){
        semantic_error(node->line_number, "Return outside function");
        return 0;
   }
   if (node->left){ 
    analyze_node(node->left);
    if (!types_compatible(node->left->data_type, node->left->pointer_level, node->left->struct_def,
                          current_function->type, current_function->pointer_level, current_function->struct_def)) {
        semantic_error(node->line_number, "Return type mismatch");
    }
   } else { 
       if(current_function->type != TYPE_VOID) semantic_error(node->line_number, "Return type mismatch");
   }
   return 1;
}

int analyze_if(ASTNode *node) {
    analyze_node(node->cond);
    if (node->cond->data_type == TYPE_VOID) semantic_error(node->line_number, "Invalid condition type");
    int then_returns = analyze_node(node->left);
    int else_returns = 0;
    if (node->right) else_returns = analyze_node(node->right);
    return (then_returns && else_returns);
}

int analyze_while(ASTNode *node) {
    analyze_node(node->cond);
    if (node->cond->data_type == TYPE_VOID) semantic_error(node->line_number, "Invalid condition type");
    break_context_depth++;
    loop_context_depth++;
    analyze_node(node->body);
    loop_context_depth--;
    break_context_depth--;
    return 0;
}

int analyze_for(ASTNode *node) {
    enter_scope(); /* NEW: for variables declared in for-loop header */
    if (node->init) analyze_node(node->init);
    if (node->cond){
        analyze_node(node->cond);
       if (node->cond->data_type == TYPE_VOID) semantic_error(node->line_number, "Invalid condition type");
    }
    if (node->incr) analyze_node(node->incr);
    break_context_depth++;
    loop_context_depth++;
    analyze_node(node->body);
    loop_context_depth--;
    break_context_depth--;
    exit_scope(); /* NEW: close loop scope */
    return 0;
}

int analyze_switch(ASTNode *node) {
    if (node->cond) {
        analyze_node(node->cond);
        if (node->cond->data_type != TYPE_INT && node->cond->data_type != TYPE_CHAR) {
            semantic_error(node->line_number, "Switch expression must be of type int or char");
        }
    }
    break_context_depth++;
    int has_default = 0;
    for (ASTNode *c = node->body; c; c = c->next) {
        if (!c) continue;
        if (c->type != NODE_CASE) {
            analyze_node(c);
            continue;
        }
        if (c->left) {
            ASTNode *expr = c->left;
            analyze_node(expr);
            if (expr->type != NODE_CONST_INT && expr->type != NODE_CONST_CHAR) {
                semantic_error(c->line_number, "Case label must be constant int or char");
            } else {
                for (ASTNode *prev = node->body; prev != c; prev = prev->next) {
                    if (prev->type == NODE_CASE && prev->left &&
                        (prev->left->type == NODE_CONST_INT || prev->left->type == NODE_CONST_CHAR)) {
                        if (prev->left->int_val == expr->int_val) {
                            semantic_error(c->line_number, "Duplicate case label in switch");
                            break;
                        }
                    }
                }
            }
        } else {
            if (has_default) semantic_error(c->line_number, "Multiple default labels in switch");
            has_default = 1;
        }
        if (c->body) analyze_list(c->body);
    }
    break_context_depth--;
    return 0;
}

static void analyze_printf_scanf(ASTNode *node, int is_scanf) {
    if (!node->left || node->left->type != NODE_STR_LIT) {
        semantic_error(node->line_number, is_scanf ? "scanf format must be a string literal" : "printf format must be a string literal");
        return;
    }

    analyze_node(node->left);
    
    int arg_count = 0;
    ASTNode *arg = node->right;
    while (arg) {
        analyze_node(arg);
        arg_count++;
        arg = arg->next;
    }

    char *fmt = node->left->str_val;
    int spec_count = 0;
    for (int i = 0; fmt[i]; i++) {
        if (fmt[i] == '%' && fmt[i+1] != '\0') {
            if (fmt[i+1] == '%') {
                i++; // Skip %%
            } else {
                spec_count++;
                // Skip everything until a conversion specifier (naive check)
                while (fmt[i+1] && !strchr("diuoxXfFeEgGaAcspn", fmt[i+1])) {
                    i++;
                }
                if (fmt[i+1]) i++;
            }
        }
    }

    if (spec_count != arg_count) {
        char buf[128];
        snprintf(buf, sizeof(buf), "%s: format string expects %d arguments, but %d were provided", 
                 is_scanf ? "scanf" : "printf", spec_count, arg_count);
        semantic_error(node->line_number, buf);
    }

    if (is_scanf) {
        arg = node->right;
        while (arg) {
            // scanf arguments must be pointers
            if (arg->pointer_level == 0 && arg->data_type != TYPE_STRUCT) {
                // Technically strings (char arrays) are ok, but here we check pointer level
                // Symbol table should have marked pointers/arrays correctly.
            }
            arg = arg->next;
        }
    }
}

void analyze_new(ASTNode *node) {
    DataType type = TYPE_INT;
    Symbol *struct_def = NULL;
    int base_pointer_level = 0;

    if (strcmp(node->str_val, "int") == 0) {
        type = TYPE_INT;
    } else if (strcmp(node->str_val, "char") == 0) {
        type = TYPE_CHAR;
    } else if (strcmp(node->str_val, "void") == 0) {
        type = TYPE_VOID;
    } else {
        Symbol *sym = lookup(node->str_val);
        if (sym && sym->kind == SYM_TYPEDEF) {
            type = sym->type;
            struct_def = sym->struct_def;
            base_pointer_level = sym->pointer_level;
        } else if (sym && sym->kind == SYM_STRUCT) {
            type = TYPE_STRUCT;
            struct_def = sym;
        } else {
            semantic_error(node->line_number, "Unknown type in new expression");
        }
    }

    node->data_type = type;
    node->struct_def = struct_def;
    node->pointer_level = base_pointer_level + 1;

    if (type == TYPE_STRUCT && struct_def) {
        char buf[256];
        snprintf(buf, sizeof(buf), "%s__ctor", struct_def->name);
        Symbol *ctor = lookup(buf);
        node->func_sym = ctor;

        ASTNode *arg = node->params;
        while (arg) {
            analyze_node(arg);
            arg = arg->next;
        }

        /* Check constructor arguments (skipping implicit 'this' in ctor->param_types[0]) */
        if (ctor && ctor->param_count > 0) {
            arg = node->params;
            int i = 0;
            while (arg && i < ctor->param_count - 1) {
                int param_idx = i + 1; // Skip 'this'
                int param_pointer_level = (ctor->param_pointer_levels ? ctor->param_pointer_levels[param_idx] : 0);
                if (!types_compatible(arg->data_type, arg->pointer_level, arg->struct_def,
                                      ctor->param_types[param_idx], param_pointer_level, NULL)) {
                    semantic_error(node->line_number, "Argument type mismatch");
                }
                arg = arg->next;
                i++;
            }
            if (i < ctor->param_count - 1) semantic_error(node->line_number, "Too few arguments to constructor");
            if (arg) semantic_error(node->line_number, "Too many arguments to constructor");
        }
    }
}

void analyze_delete(ASTNode *node) {
    analyze_node(node->left);
    if (!node->left || node->left->pointer_level == 0) {
        semantic_error(node->line_number, "delete operand must be a pointer");
        return;
    }

    if (node->left->data_type == TYPE_STRUCT && node->left->struct_def) {
        Symbol *sd = node->left->struct_def;
        char buf[256];
        snprintf(buf, sizeof(buf), "%s__dtor", sd->name);
        Symbol *dtor = lookup(buf);
        node->func_sym = dtor;
    }
    node->data_type = TYPE_VOID;
}

int analyze_node(ASTNode *node) {
    if (!node) return 0;
    switch(node->type) {
        case NODE_STRUCT_DEF: analyze_struct_def(node); return 0;
        case NODE_FUNC_DEF: analyze_function(node, NULL); return 0;
        case NODE_VAR_DECL: analyze_declaration(node); return 0;
        case NODE_ARRAY_DECL: analyze_declaration(node); return 0;
        case NODE_BLOCK: return analyze_block(node);
        case NODE_IF: return analyze_if(node);
        case NODE_WHILE: return analyze_while(node);
        case NODE_FOR: return analyze_for(node);
        case NODE_SWITCH: return analyze_switch(node);
        case NODE_RETURN: return analyze_return(node);
        case NODE_BREAK:
            if (break_context_depth <= 0) semantic_error(node->line_number, "break statement not within loop or switch");
            return 0;
        case NODE_CONTINUE:
            if (loop_context_depth <= 0) semantic_error(node->line_number, "continue statement not within loop");
            return 0;
        case NODE_ASSIGN: analyze_assignment(node); return 0;
        case NODE_BIN_OP: analyze_binary(node); return 0;
        case NODE_UN_OP: analyze_unary(node); return 0;
        case NODE_PRE_INC:
        case NODE_PRE_DEC:
        case NODE_POST_INC:
        case NODE_POST_DEC:
             analyze_increment_decrement(node);
             return 0;
        case NODE_INDEX: analyze_index(node); return 0;
        case NODE_MEMBER_ACCESS: analyze_member_access(node); return 0;
        case NODE_CONST_INT: node->data_type = TYPE_INT; return 0;
        case NODE_CONST_CHAR: node->data_type = TYPE_CHAR; return 0;
        case NODE_STR_LIT: node->data_type = TYPE_CHAR; return 0;
        case NODE_VAR: analyze_variable(node); return 0;
        case NODE_FUNC_CALL: analyze_function_call(node); return 0;
        case NODE_NEW: analyze_new(node); return 0;
        case NODE_DELETE: analyze_delete(node); return 0;
        case NODE_PRINTF: analyze_printf_scanf(node, 0); return 0;
        case NODE_SCANF: analyze_printf_scanf(node, 1); return 0;
        case NODE_TYPE:
        case NODE_EMPTY: return 0;
        default: return 0;
    }
}

int analyze_list(ASTNode *node) {
    int returns = 0;
    while (node) {
        returns = analyze_node(node);
        if(returns) break; 
        node = node->next;
    }
    return returns;
}

int get_type_size(DataType t, int pointer_level, Symbol *struct_def) {
    if (pointer_level > 0) return 8;
    if (t == TYPE_INT) return 4;
    if (t == TYPE_CHAR) return 1;
    if (t == TYPE_VOID) return 0;
    if (t == TYPE_STRUCT) {
        if (struct_def) return struct_def->struct_size;
        return 0;
    }
    return 4;
}

void semantic_analyze(ASTNode *node) {
    if (!node) return;
    init_builtin_functions();
    analyze_list(node);
}
