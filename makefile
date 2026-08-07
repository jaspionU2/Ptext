CC ?= cc
CFLAGS ?= -Wall -Wextra -pedantic -std=c99 -Iinclude

SRC = src/main.c src/editor/core/editor_state.c src/editor/file/editor_file.c src/editor/history/editor_history.c src/editor/input/editor_input.c src/editor/init/editor_init.c src/editor/render/editor_render.c src/editor/row/editor_row.c src/editor/buffer.c src/terminal/terminal.c src/utils/stack.c

main: $(SRC)
	$(CC) $(CFLAGS) $(SRC) -o main

run: main
	./src/main
