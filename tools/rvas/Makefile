CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -O2 -g -D_DEFAULT_SOURCE

OBJS = lexer.o parse.o encode.o asm.o elf_out.o main.o

rvas: $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) rvas

.PHONY: clean
