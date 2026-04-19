#ifndef PARSER_TYPEDEFS_H
#define PARSER_TYPEDEFS_H

int parser_is_typedef_name(const char *name);
void parser_register_typedef_name(const char *name);
void parser_clear_typedef_names(void);

#endif
