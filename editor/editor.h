#ifndef EDITOR_H
#define EDITOR_H

#include <time.h>
#include <stddef.h>
#include <stddef.h>
#include "buffer.h"

#define PTEXT_VERSION "0.0.1"
#define PTEXT_TAB_STOP 8

typedef struct
{
    int size;
    int rsize;
    char *chars;
    char *render;
} editorRow;

typedef struct
{
    int cursorX, cursorY;
    int renderX;
    int rowOff;
    int colOff;
    int screenrows;
    int screencols;
    int numRows;
    editorRow *row;
    char *filename;
    char statusMessage[80];
    time_t statusMsgTime;
} EditorConfig;

extern EditorConfig eConfig;
extern int drawMessageBar;

/*** init ***/
void initEditor();

/*** file i/o ***/
void editorOpen(char *filename);

/*** row operation ***/

int editorRowCxToRx(editorRow *row, int cursorX);
void editorUpdateRow(editorRow *row);
void editorAppendRow(char *s, size_t len);
void editorInsertInRow(editorRow *atRow, int atCol, int key);
void editorInsertNewline();
void editorDelChar(int atRow, int atCol);
void editorBackspaceChar(int atRow, int atCol);

/*** editor operation ***/
void editorInsertChar(int c);

/*** input ***/

void editorMoveCursor(int key);
void editorProcessKeyPress();

/*** output ***/

void editorScroll();
void editorDrawRows(abuf *appendBuffer);
void editorDrawnStatusBar(abuf *appendBuffer);
void editorDrawnMessageBar(abuf *appendBuffer);
void editorRefreshScreen();
void editorSetStatusMessage(const char *format, ...);

#endif