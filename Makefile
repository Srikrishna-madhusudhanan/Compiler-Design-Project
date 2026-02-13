CC = gcc
CFLAGS = -Wall
BISON = bison
FLEX = flex

all: parser

parser: parser.y lexer.l ast.c ast.h
	$(BISON) -t -d -o y.tab.c parser.y
	$(FLEX) lexer.l
	$(CC) $(CFLAGS) -DYYDEBUG=1 -o parser y.tab.c lex.yy.c ast.c

clean:
	rm -f parser y.tab.c y.tab.h lex.yy.c
