%{
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "ast.h"
#include "parser_typedefs.h"
#include "symbol_table.h"
#include "semantic.h"
#include "ir_gen.h"
#include "ir_opt.h"
#include "reg_alloc.h"
#include "ir_sched.h"
#include "riscv_gen.h"
#include "compiler_metrics.h"

void print_vtables() {
    Scope *global = current_scope;
    while (global && global->level > 0) global = global->parent;
    if (!global) return;
    for (int i = 0; i < TABLE_SIZE; i++) {
        Symbol *sym = global->table[i];
        while (sym) {
            if (sym->kind == SYM_STRUCT && sym->virtual_methods) {
                printf("vtable_%s:\n", sym->name);
                Symbol *m = sym->virtual_methods;
                int idx = 0;
                while (m) {
                    printf("  .word %s\n", m->name);
                    idx++;
                    m = m->next_member;
                }
            }
            sym = sym->next;
        }
    }
}

// Declarations from Flex
extern int yylex();
extern int line_num;
extern int col_num;
extern char *yytext;
extern FILE *yyin;

void yyerror(const char *s);

#define SET_LINE(n) (n)->line_number = line_num;


// Global Root
ASTNode *root = NULL;

static void register_typedef_declarator_list(ASTNode *list) {
    for (ASTNode *n = list; n; n = n->next) {
        if (n->str_val && *n->str_val) {
            parser_register_typedef_name(n->str_val);
        }
    }
}

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

static int is_identifier_text(const char *s) {
    if (!s || !*s) return 0;
    if (!(isalpha((unsigned char)s[0]) || s[0] == '_')) return 0;
    for (int i = 1; s[i]; ++i) {
        if (!(isalnum((unsigned char)s[i]) || s[i] == '_')) return 0;
    }
    return 1;
}

static const char *closest_keyword(const char *tok, int *out_dist) {
    static const char *kws[] = {
        "int", "char", "void", "const", "typedef", "struct", "class", "virtual",
        "public", "private", "if", "else", "while", "for", "return", "switch",
        "case", "default", "break", "continue", "printf", "scanf", "new", "delete",
        "try", "catch", "throw", NULL
    };

    const char *best = NULL;
    int best_dist = 999;
    for (int i = 0; kws[i]; ++i) {
        int d = edit_distance_ci(tok, kws[i]);
        if (d < best_dist) {
            best_dist = d;
            best = kws[i];
        }
    }
    if (out_dist) *out_dist = best_dist;
    return best;
}

static int token_can_start_statement(const char *tok) {
    if (!tok || !*tok) return 0;
    static const char *starters[] = {
        "int", "char", "void", "const", "typedef", "if", "while", "for", "return",
        "switch", "break", "continue", "printf", "scanf", "try", "throw", "}",
        "else", "case", "default", NULL
    };
    for (int i = 0; starters[i]; ++i) {
        if (strcmp(tok, starters[i]) == 0) return 1;
    }
    return 0;
}
%}

%union {
    int intval;
    char *str;
    struct ASTNode *node;
}

/* Tokens */
%token <intval> T_INT T_VOID T_CHAR T_STRUCT T_VIRTUAL T_CLASS T_PUBLIC T_PRIVATE T_COLON
%token <intval> T_IF T_ELSE T_WHILE T_FOR T_RETURN T_SWITCH T_CASE T_DEFAULT T_BREAK T_CONTINUE T_PRINTF T_SCANF T_CONST T_TRY T_CATCH T_THROW T_NEW T_DELETE T_TYPEDEF
%token <str>    T_IDENT T_TYPE_NAME T_STRING_LIT
%token <intval> T_NUMBER T_CHAR_LIT
%token <intval> T_ARROW T_TILDE

/* Operators */
%token <intval> T_EQ T_NEQ T_LE T_GE T_AND T_OR T_INC T_DEC

/* Precedence (lowest to highest) */
%right '='
%left T_OR
%left T_AND
%left T_EQ T_NEQ
%left '<' '>' T_LE T_GE
%left '+' '-'
%left '*' '/' '%'
%right '!'
%nonassoc LOWER_THAN_ELSE
%nonassoc T_ELSE

/* Types for non-terminals */
%type <node> program external_declaration_list external_declaration
%type <node> function_definition parameter_list parameter_declaration
%type <node> declaration declarator_list init_declarator declarator pointer direct_declarator type_specifier
%type <node> statement compound_statement block_item_list block_item
%type <node> expression_statement selection_statement iteration_statement jump_statement
%type <node> switch_statement switch_clause_list switch_clause
%type <node> statement_list statement_list_opt
%type <node> expression assignment_expression logical_or_expression logical_and_expression
%type <node> equality_expression relational_expression additive_expression
%type <node> multiplicative_expression unary_expression postfix_expression primary_expression
%type <node> argument_expression_list
%type <node> try_statement catch_clause_list catch_clause throw_statement
%type <node> struct_specifier struct_declaration_list struct_member class_specifier
%type <str> class_head

%%

/* Grammar Rules */

program
    : external_declaration_list { root = $1; }
    ;

external_declaration_list
    : external_declaration { $$ = $1; }
    | external_declaration_list external_declaration {
        $$ = append_node($1, $2);
    }
    ;

external_declaration
    : function_definition { $$ = $1; }
    | declaration { $$ = $1; }
    | error ';' {
          yyerrok;
          $$ = NULL;
      }
    ;

/* Function Definition */
function_definition
    : type_specifier T_IDENT '(' parameter_list ')' compound_statement {
        $$ = create_func_def($1, $2, $4, $6);
        SET_LINE($$);
    }
    | type_specifier T_IDENT '(' ')' compound_statement {
        $$ = create_func_def($1, $2, NULL, $5);
        SET_LINE($$);
    }
    | type_specifier pointer T_IDENT '(' parameter_list ')' compound_statement {
        $1->pointer_level = $2->int_val;
        $$ = create_func_def($1, $3, $5, $7);
        SET_LINE($$);
    }
    | type_specifier pointer T_IDENT '(' ')' compound_statement {
        $1->pointer_level = $2->int_val;
        $$ = create_func_def($1, $3, NULL, $6);
        SET_LINE($$);
    }
    ;

parameter_list
    : parameter_declaration { $$ = $1; }
    | parameter_list ',' parameter_declaration {
        $$ = append_node($1, $3);
    }
    ;

parameter_declaration
    : type_specifier declarator {
        ASTNode *param = create_node(NODE_PARAM);
        SET_LINE(param);
        param->left = $1;
        param->str_val = $2->str_val;
        param->pointer_level = $2->pointer_level;
        param->array_dim_count = $2->array_dim_count;
        param->array_dim_exprs = $2->array_dim_exprs;
        // Note: $2 is freed implicitly or we can free it
        $$ = param;
    }
    ;

/* Declarations */
declaration
    : type_specifier declarator_list ';' {
        /* If this is a struct definition with declarators (e.g. "struct S { ... } a;")
         * we need to keep the struct definition node in the AST so semantic
         * analysis can register the struct type before processing the variables.
         */
        ASTNode *result = NULL;
        ASTNode *type_node = NULL;

        if ($1->type == NODE_STRUCT_DEF) {
            /* Keep the struct definition in the list */
            result = $1;
            /* Build a type node to attach to each declarator */
            type_node = create_type_node(T_STRUCT);
            type_node->str_val = strdup($1->str_val);
            type_node->left = $1; /* keep definition for semantic use */
        } else {
            /* Normal type specifier (int/char/void/struct ref) */
            type_node = $1;
        }

        /* Distribute type to all declarators */
        ASTNode *temp = $2;
        while(temp) {
            temp->left = type_node;
            if (type_node->type == NODE_TYPE && type_node->int_val == T_STRUCT) {
                /* Preserve struct tag name if set */
                if (!temp->left->str_val && type_node->str_val)
                    temp->left->str_val = strdup(type_node->str_val);
            }
            temp = temp->next;
        }

        if (result) {
            /* append declarators after struct definition */
            $$ = append_node(result, $2);
        } else {
            $$ = $2;
        }
    }
    | T_CONST type_specifier declarator_list ';' {
        ASTNode *result = NULL;
        ASTNode *type_node = NULL;

        if ($2->type == NODE_STRUCT_DEF) {
            result = $2;
            type_node = create_type_node(T_STRUCT);
            type_node->str_val = strdup($2->str_val);
            type_node->left = $2;
        } else {
            type_node = $2;
        }

        ASTNode *temp = $3;
        while(temp) {
            temp->left = type_node;
            temp->is_const = 1; /* Mark as const */
            if (type_node->type == NODE_TYPE && type_node->int_val == T_STRUCT) {
                if (!temp->left->str_val && type_node->str_val)
                    temp->left->str_val = strdup(type_node->str_val);
            }
            temp = temp->next;
        }

        if (result) {
            $$ = append_node(result, $3);
        } else {
            $$ = $3;
        }
    }
    | T_TYPEDEF type_specifier declarator_list ';' {
        ASTNode *result = NULL;
        ASTNode *type_node = NULL;

        if ($2->type == NODE_STRUCT_DEF) {
            result = $2;
            type_node = create_type_node(T_STRUCT);
            type_node->str_val = strdup($2->str_val);
            type_node->left = $2;
        } else {
            type_node = $2;
        }

        ASTNode *temp = $3;
        while(temp) {
            temp->left = type_node;
            temp->is_typedef = 1;
            if (type_node->type == NODE_TYPE && type_node->int_val == T_STRUCT) {
                if (!temp->left->str_val && type_node->str_val)
                    temp->left->str_val = strdup(type_node->str_val);
            }
            temp = temp->next;
        }

        if (result) {
            $$ = append_node(result, $3);
        } else {
            $$ = $3;
        }
        register_typedef_declarator_list($3);
    }
    | T_TYPEDEF T_CONST type_specifier declarator_list ';' {
        ASTNode *result = NULL;
        ASTNode *type_node = NULL;

        if ($3->type == NODE_STRUCT_DEF) {
            result = $3;
            type_node = create_type_node(T_STRUCT);
            type_node->str_val = strdup($3->str_val);
            type_node->left = $3;
        } else {
            type_node = $3;
        }

        ASTNode *temp = $4;
        while(temp) {
            temp->left = type_node;
            temp->is_typedef = 1;
            temp->is_const = 1;
            if (type_node->type == NODE_TYPE && type_node->int_val == T_STRUCT) {
                if (!temp->left->str_val && type_node->str_val)
                    temp->left->str_val = strdup(type_node->str_val);
            }
            temp = temp->next;
        }

        if (result) {
            $$ = append_node(result, $4);
        } else {
            $$ = $4;
        }
        register_typedef_declarator_list($4);
    }
    | type_specifier declarator_list error {
        yyerrok;
        $$ = $2;
      }
    | struct_specifier ';' {
        /* Standalone struct definition (no variable declared) */
        $$ = $1;
    }
    | class_specifier ';' {
        /* Standalone class definition (no variable declared) */
        $$ = $1;
    }
    ;

declarator_list
    : init_declarator { $$ = $1; }
    | declarator_list ',' init_declarator {
        if ($3)
          $$ = append_node($1, $3);
        else
          $$ = $1;
    }
    ;

init_declarator
    : declarator { $$ = $1; }
    | declarator '=' expression {
        $1->right = $3;   /* attach initializer */
        $$ = $1;
    }
    ;

declarator
    : pointer direct_declarator {
        $$ = $2;
        $$->pointer_level = $1->int_val;
    }
    | direct_declarator {
        $$ = $1;
        $$->pointer_level = 0;
    }
    ;

pointer
    : '*' {
        $$ = create_node(NODE_TYPE);
        $$->int_val = 1;
    }
    | '*' pointer {
        $$ = $2;
        $$->int_val++;
    }
    ;

direct_declarator
    : T_IDENT {
        $$ = create_node(NODE_VAR_DECL);
        SET_LINE($$);
        $$->str_val = strdup($1);
        $$->array_dim_count = 0;
        $$->array_dim_exprs = NULL;
        $$->next = NULL;
    }
    | direct_declarator '[' expression ']' {
        $$ = $1;
        $$->array_dim_exprs = realloc($$->array_dim_exprs, sizeof(ASTNode*) * ($$->array_dim_count + 1));
        $$->array_dim_exprs[$$->array_dim_count] = $3;
        $$->array_dim_count++;
    }
    | direct_declarator '[' ']' {
        $$ = $1;
        $$->array_dim_exprs = realloc($$->array_dim_exprs, sizeof(ASTNode*) * ($$->array_dim_count + 1));
        $$->array_dim_exprs[$$->array_dim_count] = NULL; // VLA
        $$->array_dim_count++;
    }
    ;

type_specifier: T_INT { $$ = create_type_node(T_INT); }
               | T_CHAR { $$ = create_type_node(T_CHAR); }
               | T_VOID { $$ = create_type_node(T_VOID); }
               | struct_specifier { $$ = $1; }
               | class_specifier { $$ = $1; }
               | T_TYPE_NAME {
                   $$ = create_node(NODE_TYPE); 
                   $$->str_val = strdup($1);
                   $$->data_type = TYPE_STRUCT;
                   SET_LINE($$);
               }
               ;

struct_specifier
    : T_STRUCT T_IDENT {
        /* Make struct tag visible as a type while parsing its body. */
        parser_register_typedef_name($2);
    } '{' struct_declaration_list '}' {
        ASTNode *node = create_node(NODE_STRUCT_DEF);
        SET_LINE(node);
        node->str_val = strdup($2);             /* struct tag */
        node->body = $5;                        /* member declarations */
        $$ = node;
    }
    | T_STRUCT T_IDENT {
        /* Struct type reference (no definition)
         * We create a NODE_TYPE with int_val = T_STRUCT and str_val = tag.
         */
        parser_register_typedef_name($2);
        ASTNode *node = create_type_node(T_STRUCT);
        node->str_val = strdup($2);
        $$ = node;
    }
    ;

class_head
    : T_CLASS T_IDENT {
        /* Make class tag visible as a type while parsing its body. */
        parser_register_typedef_name($2);
        $$ = $2;
    }
    ;

class_specifier
    : class_head '{' struct_declaration_list '}' {
        ASTNode *node = create_node(NODE_STRUCT_DEF);
        SET_LINE(node);
        node->str_val = strdup($1);
        node->body = $3;
        node->is_class = 1;
        $$ = node;
    }
    | class_head T_COLON T_PUBLIC T_IDENT '{' struct_declaration_list '}' {
        ASTNode *node = create_node(NODE_STRUCT_DEF);
        SET_LINE(node);
        node->str_val = strdup($1);
        node->base_class_name = strdup($4);
        node->body = $6;
        node->is_class = 1;
        node->inheritance_modifier = 0; /* public */
        $$ = node;
    }
    | class_head T_COLON T_PRIVATE T_IDENT '{' struct_declaration_list '}' {
        ASTNode *node = create_node(NODE_STRUCT_DEF);
        SET_LINE(node);
        node->str_val = strdup($1);
        node->base_class_name = strdup($4);
        node->body = $6;
        node->is_class = 1;
        node->inheritance_modifier = 1; /* private */
        $$ = node;
    }
    | class_head T_COLON T_IDENT '{' struct_declaration_list '}' {
        ASTNode *node = create_node(NODE_STRUCT_DEF);
        SET_LINE(node);
        node->str_val = strdup($1);
        node->base_class_name = strdup($3);
        node->body = $5;
        node->is_class = 1;
        node->inheritance_modifier = 1; /* DEFAULT private for class */
        $$ = node;
    }
    | class_head {
        ASTNode *node = create_type_node(T_CLASS);
        node->str_val = strdup($1);
        $$ = node;
    }
    ;

struct_declaration_list
    : struct_member { $$ = $1; }
    | struct_declaration_list struct_member { $$ = append_node($1, $2); }
    ;

struct_member
    : declaration { $$ = $1; }
    | T_VIRTUAL function_definition {
        /* Mark the function as virtual */
        $2->is_virtual = 1;
        $$ = $2;
    }
    | function_definition { $$ = $1; }
    | T_IDENT '(' parameter_list ')' compound_statement {
        /* Constructor with params */
        $$ = create_func_def(create_type_node(T_VOID), $1, $3, $5);
        $$->is_constructor = 1;
        SET_LINE($$);
    }
    | T_TYPE_NAME '(' parameter_list ')' compound_statement {
        /* Constructor with params where class name lexes as type name */
        $$ = create_func_def(create_type_node(T_VOID), $1, $3, $5);
        $$->is_constructor = 1;
        SET_LINE($$);
    }
    | T_IDENT '(' ')' compound_statement {
        /* Constructor without params */
        $$ = create_func_def(create_type_node(T_VOID), $1, NULL, $4);
        $$->is_constructor = 1;
        SET_LINE($$);
    }
    | T_TYPE_NAME '(' ')' compound_statement {
        /* Constructor without params where class name lexes as type name */
        $$ = create_func_def(create_type_node(T_VOID), $1, NULL, $4);
        $$->is_constructor = 1;
        SET_LINE($$);
    }
    | T_TILDE T_IDENT '(' ')' compound_statement {
        /* Destructor */
        $$ = create_func_def(create_type_node(T_VOID), $2, NULL, $5);
        $$->is_destructor = 1;
        SET_LINE($$);
    }
    | T_TILDE T_TYPE_NAME '(' ')' compound_statement {
        /* Destructor where class name lexes as type name */
        $$ = create_func_def(create_type_node(T_VOID), $2, NULL, $5);
        $$->is_destructor = 1;
        SET_LINE($$);
    }
    | T_PUBLIC T_COLON {
        ASTNode *node = create_node(NODE_ACCESS_SPEC);
        SET_LINE(node);
        node->access_modifier = 0; /* public */
        $$ = node;
    }
    | T_PRIVATE T_COLON {
        ASTNode *node = create_node(NODE_ACCESS_SPEC);
        SET_LINE(node);
        node->access_modifier = 1; /* private */
        $$ = node;
    }
    ;

/* Statements */
statement
    : compound_statement { $$ = $1; }
    | expression_statement { $$ = $1; }
    | selection_statement { $$ = $1; }
    | iteration_statement { $$ = $1; }
    | jump_statement { $$ = $1; }
    | switch_statement { $$ = $1; }
    | try_statement { $$ = $1; }
    | throw_statement { $$ = $1; }
    | T_PRINTF '(' T_STRING_LIT ')' ';' {
        ASTNode *fmt = create_str_node($3);
        SET_LINE(fmt);
        $$ = create_printf_node(fmt, NULL);
        SET_LINE($$);
    }
    | T_PRINTF '(' T_STRING_LIT ',' argument_expression_list ')' ';' {
        ASTNode *fmt = create_str_node($3);
        SET_LINE(fmt);
        $$ = create_printf_node(fmt, $5);
        SET_LINE($$);
    }
    | T_SCANF '(' T_STRING_LIT ',' argument_expression_list ')' ';' {
        ASTNode *fmt = create_str_node($3);
        SET_LINE(fmt);
        $$ = create_scanf_node(fmt, $5);
        SET_LINE($$);
    }
    | T_DELETE expression ';' {
        $$ = create_node(NODE_DELETE);
        SET_LINE($$);
        $$->left = $2;
    }
    ;

compound_statement
    : '{' '}' {
        $$ = create_node(NODE_BLOCK);
        SET_LINE($$);
    }
    | '{' block_item_list '}' {
        $$ = create_node(NODE_BLOCK);
        SET_LINE($$);
        $$->left = $2;
    }
    | '{' error '}' {
        yyerrok;
        $$ = create_node(NODE_BLOCK);
        SET_LINE($$);
      }
    ;

block_item_list
    : block_item { $$ = $1; }
    | block_item_list block_item {
        $$ = append_node($1, $2);
    }
    ;

block_item
    : declaration { $$ = $1; }
    | statement { $$ = $1; }
    ;

expression_statement
    : ';' { $$ = create_node(NODE_EMPTY);  SET_LINE($$); }
    | expression ';' { $$ = $1; }
    | error ';' {
          yyerrok;
          $$ = create_node(NODE_EMPTY);
          SET_LINE($$);
      }
    ;

selection_statement
    : T_IF '(' expression ')' statement %prec LOWER_THAN_ELSE {
        $$ = create_if_node($3, $5, NULL);
        SET_LINE($$);
    }
    | T_IF '(' expression ')' statement T_ELSE statement {
        $$ = create_if_node($3, $5, $7);
        SET_LINE($$);
    }
    ;

iteration_statement
    : T_WHILE '(' expression ')' statement {
        $$ = create_while_node($3, $5);
        SET_LINE($$);
    }
    | T_FOR '(' expression_statement expression_statement expression ')' statement {
        $$ = create_for_node($3, $4, $5, $7);
        SET_LINE($$);
    }
    | T_FOR '(' declaration expression_statement ')' statement {
        $$ = create_for_node($3, $4, NULL, $6);
        SET_LINE($$);
    }
    | T_FOR '(' declaration expression_statement expression ')' statement {
        $$ = create_for_node($3, $4, $5, $7);
        SET_LINE($$);
    }
    ;

jump_statement
    : T_RETURN ';' {
        $$ = create_node(NODE_RETURN);
        SET_LINE($$);
    }
    | T_RETURN expression ';' {
        $$ = create_node(NODE_RETURN);
        SET_LINE($$);
        $$->left = $2;
    }
    | T_BREAK ';' {
        $$ = create_break_node();
        SET_LINE($$);
    }
    | T_CONTINUE ';' {
        $$ = create_continue_node();
        SET_LINE($$);
    }
    ;

throw_statement
    : T_THROW expression ';' {
        $$ = create_throw_node($2);
        SET_LINE($$);
    }
    | T_THROW ';' {
        $$ = create_throw_node(NULL);
        SET_LINE($$);
    }
    ;

try_statement
    : T_TRY compound_statement catch_clause_list {
        $$ = create_try_node($2, $3);
        SET_LINE($$);
    }
    ;

catch_clause_list
    : catch_clause { $$ = $1; }
    | catch_clause_list catch_clause {
        $$ = append_node($1, $2);
    }
    ;

catch_clause
    : T_CATCH '(' expression ')' compound_statement {
        $$ = create_catch_node($3, $5);
        SET_LINE($$);
    }
    | T_CATCH '(' ')' compound_statement {
        $$ = create_catch_node(NULL, $4);
        SET_LINE($$);
    }
    ;

switch_statement
    : T_SWITCH '(' expression ')' '{' switch_clause_list '}' {
        $$ = create_switch_node($3, $6);
        SET_LINE($$);
    }
    ;

switch_clause_list
    : switch_clause { $$ = $1; }
    | switch_clause_list switch_clause {
        $$ = append_node($1, $2);
    }
    ;

switch_clause
    : T_CASE expression T_COLON statement_list_opt {
        $$ = create_case_node($2, $4);
        SET_LINE($$);
    }
    | T_DEFAULT T_COLON statement_list_opt {
        $$ = create_case_node(NULL, $3);
        SET_LINE($$);
    }
    ;

statement_list_opt
    : /* empty */ { $$ = NULL; }
    | statement_list { $$ = $1; }
    ;

statement_list
    : statement { $$ = $1; }
    | statement_list statement {
        $$ = append_node($1, $2);
    }
    ;

/* Expressions */
expression
    : assignment_expression { $$ = $1; }
    ;

assignment_expression
    : logical_or_expression { /* no assignment, just propagate the expression */
        $$ = $1;
    }
    /* allow any lvalue produced by logical_or_expression (identifier, index, etc.) */
    | logical_or_expression '=' assignment_expression {
        /* left side is already an ASTNode representing a variable or indexed value */
        $$ = create_node(NODE_ASSIGN);
        SET_LINE($$);
        $$->left = $1;
        $$->right = $3;
    }
    ;

logical_or_expression
    : logical_and_expression { $$ = $1; }
    | logical_or_expression T_OR logical_and_expression {
        $$ = create_binary_node(T_OR, $1, $3);
        SET_LINE($$);
    }
    ;

logical_and_expression
    : equality_expression { $$ = $1; }
    | logical_and_expression T_AND equality_expression {
        $$ = create_binary_node(T_AND, $1, $3);
        SET_LINE($$);
    }
    ;

equality_expression
    : relational_expression { $$ = $1; }
    | equality_expression T_EQ relational_expression {
        $$ = create_binary_node(T_EQ, $1, $3);
        SET_LINE($$);
    }
    | equality_expression T_NEQ relational_expression {
        $$ = create_binary_node(T_NEQ, $1, $3);
        SET_LINE($$);
    }
    ;

relational_expression
    : additive_expression { $$ = $1; }
    | relational_expression '<' additive_expression {
        $$ = create_binary_node('<', $1, $3);
        SET_LINE($$);
    }
    | relational_expression '>' additive_expression {
        $$ = create_binary_node('>', $1, $3);
        SET_LINE($$);
    }
    | relational_expression T_LE additive_expression {
        $$ = create_binary_node(T_LE, $1, $3);
        SET_LINE($$);
    }
    | relational_expression T_GE additive_expression {
        $$ = create_binary_node(T_GE, $1, $3);
        SET_LINE($$);
    }
    ;

additive_expression
    : multiplicative_expression { $$ = $1; }
    | additive_expression '+' multiplicative_expression {
        $$ = create_binary_node('+', $1, $3);
        SET_LINE($$);
    }
    | additive_expression '-' multiplicative_expression {
        $$ = create_binary_node('-', $1, $3);
        SET_LINE($$);
    }
    ;

multiplicative_expression
    : unary_expression { $$ = $1; }
    | multiplicative_expression '*' unary_expression {
        $$ = create_binary_node('*', $1, $3);
        SET_LINE($$);
    }
    | multiplicative_expression '/' unary_expression {
        $$ = create_binary_node('/', $1, $3);
        SET_LINE($$);
    }
    | multiplicative_expression '%' unary_expression {
        $$ = create_binary_node('%', $1, $3);
        SET_LINE($$);
    }
    ;

postfix_expression
    : primary_expression { $$ = $1; }
    | postfix_expression '[' expression ']' {
        $$ = create_index_node($1, $3);
        SET_LINE($$);
    }
    | postfix_expression '.' T_IDENT {
        ASTNode *node = create_node(NODE_MEMBER_ACCESS);
        SET_LINE(node);
        node->left = $1;
        node->str_val = strdup($3);
        node->int_val = 0; /* dot */
        $$ = node;
    }
    | postfix_expression T_ARROW T_IDENT {
        ASTNode *node = create_node(NODE_MEMBER_ACCESS);
        SET_LINE(node);
        node->left = $1;
        node->str_val = strdup($3);
        node->int_val = 1; /* arrow */
        $$ = node;
    }
    | postfix_expression '(' argument_expression_list ')' {
        ASTNode *func = create_node(NODE_FUNC_CALL);
        SET_LINE(func);
        func->left = $1; /* Callee */
        func->right = $3; /* Arguments */
        if ($1 && $1->type == NODE_VAR) {
            func->str_val = strdup($1->str_val);
        }
        $$ = func;
    }
    | postfix_expression '(' ')' {
        ASTNode *func = create_node(NODE_FUNC_CALL);
        SET_LINE(func);
        func->left = $1; /* Callee */
        func->right = NULL;
        if ($1 && $1->type == NODE_VAR) {
            func->str_val = strdup($1->str_val);
        }
        $$ = func;
    }
    | postfix_expression T_INC {
        $$ = create_unary_node(T_INC, $1);
        $$->type = NODE_POST_INC;
        SET_LINE($$);
    }
    | postfix_expression T_DEC {
        $$ = create_unary_node(T_DEC, $1);
        $$->type = NODE_POST_DEC;
        SET_LINE($$);
    }
    ;

unary_expression
    : postfix_expression { $$ = $1; }
    | '-' unary_expression {
        $$ = create_unary_node('-', $2);
        SET_LINE($$);
    }
    | '!' unary_expression {
        $$ = create_unary_node('!', $2);
        SET_LINE($$);
    }
    | '&' unary_expression {
        $$ = create_unary_node('&', $2);
        SET_LINE($$);
    }
    | '*' unary_expression {
        $$ = create_unary_node('*', $2);
        SET_LINE($$);
    }
    | T_INC unary_expression {
        $$ = create_unary_node(T_INC, $2);
        $$->type = NODE_PRE_INC;
        SET_LINE($$);
    }
    | T_DEC unary_expression {
        $$ = create_unary_node(T_DEC, $2);
        $$->type = NODE_PRE_DEC;
        SET_LINE($$);
    }
    | T_NEW T_IDENT '(' argument_expression_list ')' {
        $$ = create_node(NODE_NEW);
        SET_LINE($$);
        $$->str_val = strdup($2);
        $$->params = $4;
    }
    | T_NEW T_TYPE_NAME '(' argument_expression_list ')' {
        $$ = create_node(NODE_NEW);
        SET_LINE($$);
        $$->str_val = strdup($2);
        $$->params = $4;
    }
    | T_NEW T_IDENT '(' ')' {
        $$ = create_node(NODE_NEW);
        SET_LINE($$);
        $$->str_val = strdup($2);
        $$->params = NULL;
    }
    | T_NEW T_TYPE_NAME '(' ')' {
        $$ = create_node(NODE_NEW);
        SET_LINE($$);
        $$->str_val = strdup($2);
        $$->params = NULL;
    }
    | T_NEW T_IDENT {
        /* Support for 'new int' etc. without parens */
        $$ = create_node(NODE_NEW);
        SET_LINE($$);
        $$->str_val = strdup($2);
        $$->params = NULL;
    }
    | T_NEW T_TYPE_NAME {
        /* Support for 'new Type' where Type lexes as type name */
        $$ = create_node(NODE_NEW);
        SET_LINE($$);
        $$->str_val = strdup($2);
        $$->params = NULL;
    }
    ;

primary_expression
    : T_IDENT {
        $$ = create_var_node($1);
        SET_LINE($$);
    }
    | T_NUMBER {
        $$ = create_int_node($1);
        SET_LINE($$);
    }
    | T_CHAR_LIT {
        $$ = create_char_node($1);
        SET_LINE($$);
    }
    | T_STRING_LIT {
        $$ = create_str_node($1);
        SET_LINE($$);
    }
    | '(' expression ')' {
        $$ = $2;
    }
    ;

argument_expression_list
    : expression { $$ = $1; }
    | argument_expression_list ',' expression {
        $$ = append_node($1, $3);
    }
    ;

%%

int parse_errors = 0;

static int is_likely_false_positive_hint(const char *tok, const char *keyword) {
    if (!tok || !keyword) return 0;
    
    int tok_len = strlen(tok);
    int kw_len = strlen(keyword);
    
    /* 1-3 character identifiers matching to keywords of different length
     * are likely class/method names, not typos */
    if (tok_len <= 3 && kw_len != tok_len) {
        return 1;
    }
    
    /* Identifiers up to 5 characters that are 1-2 chars different from keywords
     * are likely legitimate names, not typos */
    if (tok_len <= 5) {
        int len_diff = (tok_len > kw_len) ? (tok_len - kw_len) : (kw_len - tok_len);
        if (len_diff <= 2) {
            /* This might be a false positive - a real typo would typically affect
             * the middle characters, not just length difference */
            return 1;
        }
    }
    
    /* Single character tokens are rarely typos */
    if (tok_len == 1) {
        return 1;
    }
    
    return 0;
}

void yyerror(const char *s) {
    fprintf(stderr, "Parser Error: %s at line %d, column %d (token: %s)\n", s, line_num, col_num, yytext);

    /* First priority: check if missing semicolon */
    if (token_can_start_statement(yytext)) {
        fprintf(stderr, "Hint: it looks like you may have forgotten a semicolon before '%s'.\n", yytext);
    }
    /* Second priority: check for keyword similarity, but filter out false positives */
    else if (is_identifier_text(yytext)) {
        int dist = 999;
        const char *kw = closest_keyword(yytext, &dist);
        if (kw && dist > 0 && dist <= 2) {
            /* Filter out false positives:
             * - Short identifiers (1-3 chars) matching longer keywords
             * - Single character identifiers (likely class/method names)
             * - Identifiers that look like they might be in an OOP context
             */
            if (!is_likely_false_positive_hint(yytext, kw)) {
                fprintf(stderr, "Hint: '%s' looks similar to keyword '%s'.\n", yytext, kw);
            }
        }
    }

    parse_errors++;
}

int main(int argc, char **argv) {
    int arg_idx = 1;
    int want_metrics = 0;
    OptLevel opt_level = OPT_O2;
    while (arg_idx < argc) {
        if (strcmp(argv[arg_idx], "--debug") == 0) {
            arg_idx++;
            printf("Debug mode enabled\n");
            continue;
        }
        if (strcmp(argv[arg_idx], "--metrics") == 0) {
            arg_idx++;
            want_metrics = 1;
            continue;
        }
        if (strncmp(argv[arg_idx], "-O", 2) == 0) {
            const char *lvl = argv[arg_idx] + 2;
            if (strcmp(lvl, "0") == 0)
                opt_level = OPT_O0;
            else if (strcmp(lvl, "1") == 0)
                opt_level = OPT_O1;
            else if (strcmp(lvl, "2") == 0)
                opt_level = OPT_O2;
            else {
                fprintf(stderr, "Unknown optimization level: %s (use -O0, -O1, or -O2)\n", argv[arg_idx]);
                return 1;
            }
            arg_idx++;
            continue;
        }
        break;
    }

    if (arg_idx < argc) {
        FILE *file = fopen(argv[arg_idx], "r");
        if (!file) {
            perror("Error opening file");
            return 1;
        }
        yyin = file;
    }

    printf("Parsing...\n");
    parser_clear_typedef_names();
    int parse_result = yyparse();

    if(parse_result == 0 && parse_errors == 0 && root != NULL){
        init_symbol_table();
        semantic_analyze(root);
        if (semantic_errors == 0)
        {
          print_symbol_table();
          printf("Semantic analysis successful.\n");

          IRProgram *ir = ir_generate(root);
          if (ir) {
            CompilerMetrics metrics = {0};
            if (want_metrics)
                compiler_metrics_init(&metrics);

            ir_print_program(ir);
            print_vtables();
            ir_export_to_file(ir, "ir.txt");

            if (want_metrics)
                metrics.pre_opt_ir_instructions = compiler_metrics_count_ir_instructions(ir);

            if (opt_level == OPT_O0)
                printf("Skipping IR optimization (-O0).\n");
            else
                printf("Optimizing IR...\n");
            optimize_program(ir, opt_level, want_metrics ? &metrics : NULL);
            if (opt_level == OPT_O0)
                printf("IR unchanged (optimization level O0). Optimized IR printed below:\n");
            else
                printf("Optimization complete. Optimized IR printed below:\n");
            ir_print_program(ir);
            ir_export_to_file(ir, "ir_opt.txt");

            if (want_metrics) {
                metrics.post_opt_ir_instructions = compiler_metrics_count_ir_instructions(ir);
                metrics.post_opt_basic_blocks = compiler_metrics_count_basic_blocks(ir);
            }

            /* Instruction Scheduling */
            printf("Running instruction scheduling...\n");
            IRFunc *sf = ir->funcs;
            while (sf) {
                ir_schedule_function(sf);
                char sched_json[128];
                snprintf(sched_json, sizeof(sched_json), "%s_sched.json", sf->name);
                ir_schedule_export_json(sf, sched_json);
                sf = sf->next;
            }
            printf("Instruction scheduling complete.\n");

            /* Register allocation (Chaitin's graph coloring) */
            printf("Running register allocation...\n");
            RegAllocResult **ra_results = reg_alloc_program(ir);
            printf("Register allocation complete.\n");

            if (want_metrics)
                compiler_metrics_set_spill_total(&metrics, ra_results);

            riscv_generate(ir, ra_results, "output.s");

            if (want_metrics) {
                compiler_metrics_read_assembly_lines(&metrics, "output.s");
                compiler_metrics_print_and_save(&metrics, "compiler_metrics.txt");
            }

            reg_alloc_free_all(ra_results);
            ir_free_program(ir);
          }
        }
        else{
         printf("Semantic analysis failed with %d errors.\n", semantic_errors);
        }
    }

    if (parse_result == 0) {
        printf("Parsing Done with %d errors\n", parse_errors);
        printf("AST Structure:\n");
        if (root) {
            print_ast(root, 0);
            export_ast_to_dot(root, "ast.dot");
            export_ast_to_json(root, "ast.json");
        }
        parser_clear_typedef_names();
        return 0;
    } else {
        printf("Parsing Failed\n");
        parser_clear_typedef_names();
        return 1;
     }
}
