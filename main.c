#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <termios.h>
#include <unistd.h>
#include "editor/editor.h"
#include "terminal/terminal.h"


int main(int argc, char *argv[])
{
    enableRawMode();
    initEditor();
    if (argc >= 2)
        editorOpen(argv[1]);

    editorSetStatusMessage("HELP: Ctrl-q = quit");

    while (1)
    {
        editorRefreshScreen();
        editorProcessKeyPress();
    }

    // char c;
    // while (read(STDIN_FILENO, &c, 1) == 1 && c != 'q')
    // {
    //     if (iscntrl(c))
    //     {
    //         printf("%d\r\n", c);
    //     }
    //     else
    //     {
    //         printf("%d ('%c')\r\n", c, c);
    //     }
    // }

    return 0;
}
