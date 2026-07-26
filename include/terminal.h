#ifndef TERMINAL_H
#define TERMINAL_H

#define ESC_CODE "\x1b"
#define CURSOR_POS ESC_CODE "[6n" // \x1b[6n
#define CURSOR_INVISIBLE ESC_CODE "[?25l" // \x1b[?25l
#define CURSOR_VISIBLE ESC_CODE "[?25h"   // \x1b[?25h
#define CURSOR_EDGE ESC_CODE "[999;999H"  // \x1b[999;999H

/*** macro ***/

#define CTRL_KEY(k) ((k) & 0x1f)

enum terminalKey
{
    BACKSPACE = 127,
    ARROW_LEFT = 1000,
    ARROW_RIGHT,
    ARROW_UP,
    ARROW_DOWN,
    PAGE_UP,
    PAGE_DOWN,
    HOME_KEY,
    END_KEY,
    DEL_KEY,
};

void die(const char *s);
void disableRawMode();
void enableRawMode();
int terminalReadKey();
int getCursorPosition(int *rows, int *cols);
int getWindowSize(int *rows, int *cols);
void moveCursor(int row, int col);
void clearScreen(const char arg);
void clearLine(const char arg);

#endif
