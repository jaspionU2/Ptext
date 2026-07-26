CC ?= cc
CFLAGS ?= -Wall -Wextra -pedantic -std=c99 -Iinclude

SRC = src/main.c src/editor/editor.c src/editor/buffer.c src/terminal/terminal.c src/utils/stack.c

main: $(SRC)
	$(CC) $(CFLAGS) $(SRC) -o main

run: main
	./src/main
