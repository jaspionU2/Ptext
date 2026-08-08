#define _DEFAULT_SOURCE
#define _BSD_SOURCE
#define _GNU_SOURCE

#include <stdlib.h>

#include "editor.h"
#include "terminal.h"

void editorMoveCursor(int key)
{
    EditorRow *row = (eConfig.cursorY >= eConfig.numRows) ? NULL : &eConfig.row[eConfig.cursorY];

    switch (key)
    {
    case ARROW_UP:
        if (eConfig.cursorY != 0)
            eConfig.cursorY--;
        break;
    case ARROW_DOWN:
        if (eConfig.cursorY < eConfig.numRows)
            eConfig.cursorY++;
        break;
    case ARROW_LEFT:
        if (eConfig.cursorX > 0)
        {
            eConfig.cursorX--;
        }
        else if (eConfig.cursorY > 0)
        {
            eConfig.cursorY -= 1;
            eConfig.cursorX = eConfig.row[eConfig.cursorY].size;
        }
        break;
    case ARROW_RIGHT:
        if (row)
        {
            if (eConfig.cursorX < row->size)
                eConfig.cursorX++;
            else if (eConfig.cursorY + 1 < eConfig.numRows)
            {
                eConfig.cursorX = 0;
                eConfig.cursorY++;
            }
        }
        break;

    default:
        break;
    }

    row = (eConfig.cursorY >= eConfig.numRows) ? NULL : &eConfig.row[eConfig.cursorY];
    int rowLen = row ? row->size : 0;
    if (eConfig.cursorX > rowLen)
        eConfig.cursorX = rowLen;
}

void editorProcessKeyPress()
{
    int c = terminalReadKey();
    int deletedChar;

    if (c != CTRL_KEY('q'))
        eConfig.quitConfirm = 0;

    switch (c)
    {
    case CTRL_KEY('q'):
    {
        int isDirty = (eConfig.activeCommand.command != NULL || eConfig.undoStack != eConfig.lastSave);

        if (isDirty)
        {
            if (!eConfig.quitConfirm)
            {
                eConfig.quitConfirm = 1;
                editorSetStatusMessage("WARNING: There are unsaved changes. Press Ctrl-Q again to exit without saving, or Ctrl-S to save.");
                break;
            }
        }

        clearScreen('2');
        moveCursor(0, 0);
        exit(0);
        break;
    }
    
    case CTRL_KEY('s'):
        editorSave();
        break;

    case CTRL_KEY('t'):
        eConfig.isComandBar = eConfig.isComandBar ? 0 : 1;
        break;

    case CTRL_KEY('z'):
        editorUndo();
        break;

    case CTRL_KEY('y'):
        editorRedo();
        break;

    case (int)'\r':
    case (int)'\n':
        editorBackup("newline", c);
        editorInsertNewline();
        editorMoveCursor(ARROW_DOWN);
        eConfig.cursorX = 0;
        break;

    case HOME_KEY:
    case END_KEY:
        if (c == HOME_KEY)
        {
            eConfig.cursorX = 0;
        }
        else if (c == END_KEY)
        {
            if (eConfig.cursorY < eConfig.numRows)
                eConfig.cursorX = eConfig.row[eConfig.cursorY].size;
        }
        break;

    case BACKSPACE:
    case CTRL_KEY('h'):
    case DEL_KEY:
        if (c == DEL_KEY)
        {
            deletedChar = eConfig.row[eConfig.cursorY].chars[eConfig.cursorX];
            editorBackup("delete", deletedChar);
            editorDelChar(eConfig.cursorY, eConfig.cursorX);
        }
        else if (c == BACKSPACE || c == 8)
        {
            deletedChar = eConfig.cursorX > 0 ? eConfig.row[eConfig.cursorY].chars[eConfig.cursorX - 1] : -1;
            if (eConfig.cursorY > 0 && eConfig.cursorX == 0 && deletedChar == -1)
            {
                editorBackup("mergeLine", deletedChar);
            }
            else
            {
                editorBackup("backspace", deletedChar);
            }
            editorBackspaceChar(eConfig.cursorY, eConfig.cursorX);
        }
        break;

    case PAGE_UP:
    case PAGE_DOWN:
    {
        if (c == PAGE_UP)
        {
            eConfig.cursorY = eConfig.rowOff;
        }
        else if (c == PAGE_DOWN)
        {
            eConfig.cursorY = eConfig.rowOff + eConfig.screenrows - 1;
            if (eConfig.cursorY > eConfig.numRows)
                eConfig.cursorY = eConfig.numRows;
        }

        int times = eConfig.screenrows;
        while (times--)
            editorMoveCursor(c == PAGE_UP ? ARROW_UP : ARROW_DOWN);
        break;
    }

    case ARROW_UP:
    case ARROW_DOWN:
    case ARROW_LEFT:
    case ARROW_RIGHT:
        editorMoveCursor(c);
        break;

    default:
        editorBackup("add", c);
        editorInsertChar(c);
        editorMoveCursor(ARROW_RIGHT);
        break;
    }
}