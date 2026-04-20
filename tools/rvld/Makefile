CC = gcc
CFLAGS = -Wall -g
SRCS = main.c elf_parse.c linker.c reloc.c elf_write.c archive.c
OBJS = $(SRCS:.c=.o)
TARGET = rvld

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS)

%.o: %.c rvld.h
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)
