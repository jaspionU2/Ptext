#ifndef EDITOR_H
#define EDITOR_H

#include <time.h>
#include <stddef.h>
#include <stddef.h>
#include "buffer.h"
#include "stack.h"

#define PTEXT_VERSION "0.0.1"
#define PTEXT_TAB_STOP 8

typedef struct
{
    char *typeAction;
    char *command;
    int row;
    int startX;
    int actualPosition;
    size_t len;
} CommandHistory;

enum actionType
{
    ADD_ACTION,
    DELETE_ACTION,
    BACKSPACE_ACTION,
    NEWLINE_ACTION
};

typedef struct
{
    int size;
    int rsize;
    char *chars;
    char *render;
} EditorRow;

typedef struct
{
    int cursorX, cursorY;
    int renderX;
    int rowOff;
    int colOff;
    int screenrows;
    int screencols;
    int numRows;
    EditorRow *row;
    char *filename;
    char statusMessage[150];
    time_t statusMsgTime;
    char comandBar[20][20];
    int isComandBar;
    int quitConfirm;
    stack *undoStack;
    stack *redoStack;
    CommandHistory activeCommand;
    stack *lastSave;
} EditorConfig;

extern EditorConfig eConfig;
extern int drawMessageBar;

/*** init ***/
void initEditor();

/*** file i/o ***/
void editorNewFile(char *filename);
void editorOpen(char *filename);
char *editorRowToString(int *buflen);
void editorSave();

/*** row operation ***/

int editorRowCxToRx(EditorRow *row, int cursorX);
void editorUpdateRow(EditorRow *row);
void editorAppendRow(char *s, size_t len);
void editorInsertInRow(EditorRow *atRow, int atCol, int key);
void editorInsertNewline();
void editorDelChar(int atRow, int atCol);
void editorBackspaceChar(int atRow, int atCol);

/*** editor operation ***/

void editorInsertChar(int c);
void editorHistoryToUndo(CommandHistory *h);
void editorBackup(char *actionType, int command);
void editorUndo();
void editorRedo();

/*** input ***/

void editorMoveCursor(int key);
void editorProcessKeyPress();

/*** output ***/

void editorScroll();
void editorDrawRows(abuf *appendBuffer);
void editorDrawnStatusBar(abuf *appendBuffer);
void editorDrawnMessageBar(abuf *appendBuffer);
void editorDrawnHelpBar(abuf *appendBuffer, char arr[][20], int numItems, int padding, int boxOfDraw);
void editorRefreshScreen();
void editorSetStatusMessage(const char *format, ...);

#endif