CC = gcc
CFLAGS = -Wall -g
BISON = bison
FLEX = flex

OBJS = y.tab.o lex.yy.o ast.o symbol_table.o semantic.o ir.o ir_gen.o

all: parser

parser: $(OBJS)
	$(CC) $(CFLAGS) -DYYDEBUG=1 -o parser $(OBJS)

y.tab.c y.tab.h: parser.y
	$(BISON) -t -d -o y.tab.c parser.y

lex.yy.c: lexer.l y.tab.h
	$(FLEX) lexer.l

y.tab.o: y.tab.c
	$(CC) $(CFLAGS) -c y.tab.c

lex.yy.o: lex.yy.c
	$(CC) $(CFLAGS) -c lex.yy.c

ast.o: ast.c ast.h
	$(CC) $(CFLAGS) -c ast.c

symbol_table.o: symbol_table.c symbol_table.h
	$(CC) $(CFLAGS) -c symbol_table.c

semantic.o: semantic.c semantic.h symbol_table.h ast.h
	$(CC) $(CFLAGS) -c semantic.c

ir.o: ir.c ir.h symbol_table.h ast.h
	$(CC) $(CFLAGS) -c ir.c

ir_gen.o: ir_gen.c ir_gen.h ir.h ast.h symbol_table.h
	$(CC) $(CFLAGS) -c ir_gen.c

clean:
	rm -f parser *.o y.tab.c y.tab.h lex.yy.c ast.dot ir.txt
