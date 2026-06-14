CC ?= cc
CFLAGS ?= -Wall -Wextra -pedantic -std=c99 -Ieditor -Iterminal

SRC = main.c editor/editor.c editor/buffer.c terminal/terminal.c

main: $(SRC)
	$(CC) $(CFLAGS) $(SRC) -o main

run: main
	./main
