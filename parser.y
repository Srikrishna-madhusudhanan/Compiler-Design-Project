%{
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Declare stuff from Flex
extern int yylex();
extern int line_num;
extern int col_num;
extern char *yytext;
extern FILE *yyin;

void yyerror(const char *s);
%}

%union {
    int intval;
    char *str;
}

/* Tokens */
%token <intval> T_INT T_VOID T_CHAR
%token <intval> T_IF T_ELSE T_WHILE T_FOR T_RETURN
%token <str>    T_IDENT T_STRING_LIT
%token <intval> T_NUMBER T_CHAR_LIT

/* Operators */
%token T_EQ T_NEQ T_LE T_GE T_AND T_OR

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

%%

/* Grammar Rules */

program
    : external_declaration_list
    ;

external_declaration_list
    : external_declaration
    | external_declaration_list external_declaration
    ;

external_declaration
    : function_definition
    | declaration
    ;

/* Function Definition */
function_definition
    : type_specifier T_IDENT '(' parameter_list ')' compound_statement
    | type_specifier T_IDENT '(' ')' compound_statement
    ;

parameter_list
    : parameter_declaration
    | parameter_list ',' parameter_declaration
    ;

parameter_declaration
    : type_specifier T_IDENT
    ;

/* Declarations */
declaration
    : type_specifier declarator_list ';'
    ;

declarator_list
    : declarator
    | declarator_list ',' declarator
    ;

declarator
    : T_IDENT
    | T_IDENT '=' expression
    ;

type_specifier
    : T_INT
    | T_VOID
    | T_CHAR
    ;

/* Statements */
statement
    : compound_statement
    | expression_statement
    | selection_statement
    | iteration_statement
    | jump_statement
    ;

compound_statement
    : '{' '}'
    | '{' block_item_list '}'
    ;

block_item_list
    : block_item
    | block_item_list block_item
    ;

block_item
    : declaration
    | statement
    ;

expression_statement
    : ';'
    | expression ';'
    ;

selection_statement
    : T_IF '(' expression ')' statement %prec LOWER_THAN_ELSE
    | T_IF '(' expression ')' statement T_ELSE statement
    ;

iteration_statement
    : T_WHILE '(' expression ')' statement
    | T_FOR '(' expression_statement expression_statement ')' statement
    | T_FOR '(' expression_statement expression_statement expression ')' statement
    ;

jump_statement
    : T_RETURN ';'
    | T_RETURN expression ';'
    ;

/* Expressions */
expression
    : assignment_expression
    ;

assignment_expression
    : logical_or_expression
    | T_IDENT '=' assignment_expression
    ;

logical_or_expression
    : logical_and_expression
    | logical_or_expression T_OR logical_and_expression
    ;

logical_and_expression
    : equality_expression
    | logical_and_expression T_AND equality_expression
    ;

equality_expression
    : relational_expression
    | equality_expression T_EQ relational_expression
    | equality_expression T_NEQ relational_expression
    ;

relational_expression
    : additive_expression
    | relational_expression '<' additive_expression
    | relational_expression '>' additive_expression
    | relational_expression T_LE additive_expression
    | relational_expression T_GE additive_expression
    ;

additive_expression
    : multiplicative_expression
    | additive_expression '+' multiplicative_expression
    | additive_expression '-' multiplicative_expression
    ;

multiplicative_expression
    : unary_expression
    | multiplicative_expression '*' unary_expression
    | multiplicative_expression '/' unary_expression
    | multiplicative_expression '%' unary_expression
    ;

unary_expression
    : primary_expression
    | '-' unary_expression
    | '!' unary_expression
    ;

primary_expression
    : T_IDENT
    | T_NUMBER
    | T_CHAR_LIT
    | T_STRING_LIT
    | '(' expression ')'
    | T_IDENT '(' argument_expression_list ')'
    | T_IDENT '(' ')'
    ;

argument_expression_list
    : expression
    | argument_expression_list ',' expression
    ;

%%

#define YYDEBUG 1
void yyerror(const char *s) {
    fprintf(stderr, "Parser Error: %s at line %d, column %d (token: %s)\n", s, line_num, col_num, yytext);
}

int main(int argc, char **argv) {
    // Check for --debug flag
    int arg_idx = 1;
    if (argc > 1 && strcmp(argv[1], "--debug") == 0) {
        yydebug = 1;
        arg_idx++;
        printf("Debug mode enabled\n");
    }

    // Check for filename
    if (arg_idx < argc) {
        FILE *file = fopen(argv[arg_idx], "r");
        if (!file) {
            perror("Error opening file");
            return 1;
        }
        yyin = file;
    }

    printf("Parsing...\n");
    if (yyparse() == 0) {
        printf("Parsing Successful\n");
        return 0;
    } else {
        printf("Parsing Failed\n");
        return 1;
    }
}