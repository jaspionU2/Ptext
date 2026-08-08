#define _DEFAULT_SOURCE
#define _BSD_SOURCE
#define _GNU_SOURCE

#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "buffer.h"
#include "editor.h"
#include "terminal.h"

void editorScroll()
{
    eConfig.renderX = 0;
    if (eConfig.cursorY < eConfig.numRows)
    {
        eConfig.renderX = editorRowCxToRx(&eConfig.row[eConfig.cursorY], eConfig.cursorX);
    }

    if (eConfig.cursorY < eConfig.rowOff)
    {
        eConfig.rowOff = eConfig.cursorY;
    }
    if (eConfig.cursorY >= eConfig.rowOff + eConfig.screenrows)
    {
        eConfig.rowOff = eConfig.cursorY - eConfig.screenrows + 1;
    }
    if (eConfig.renderX < eConfig.colOff)
    {
        eConfig.colOff = eConfig.renderX;
    }
    if (eConfig.renderX >= eConfig.colOff + eConfig.screencols)
    {
        eConfig.colOff = eConfig.renderX - eConfig.screencols + 1;
    }
}

void editorDrawRows(abuf *appendBuffer)
{
    int y;
    for (y = 0; y < eConfig.screenrows; y++)
    {
        int fileRows = y + eConfig.rowOff;
        if (fileRows >= eConfig.numRows)
        {
            if (eConfig.numRows == 0 && y == eConfig.screenrows / 3)
            {
                char welcome[80];
                int welcomeLen = snprintf(welcome, sizeof(welcome), "Ptext editor -- version %s", PTEXT_VERSION);
                if (welcomeLen > eConfig.screencols)
                    welcomeLen = eConfig.screencols;
                int padding = (eConfig.screencols - welcomeLen) / 2;

                if (padding)
                {
                    abAppend(appendBuffer, "~", 1);
                    padding--;
                }
                while (padding--)
                    abAppend(appendBuffer, " ", 1);
                abAppend(appendBuffer, welcome, welcomeLen);
            }
            else
            {
                abAppend(appendBuffer, "~", 1);
            }
        }
        else
        {
            int len = eConfig.row[fileRows].rsize - eConfig.colOff;
            if (len < 0)
                len = 0;
            if (len > eConfig.screencols)
                len = eConfig.screencols;
            abAppend(appendBuffer, &eConfig.row[fileRows].render[eConfig.colOff], len);
        }

        abAppend(appendBuffer, "\x1b[K", 3);
        abAppend(appendBuffer, "\r\n", 2);
    }
}

void editorDrawnStatusBar(abuf *appendBuffer)
{
    abAppend(appendBuffer, "\x1b[7m", 4);

    char status[80], rStatus[80], cStatus[80];

    int isDirty = (eConfig.activeCommand.command != NULL || eConfig.undoStack != eConfig.lastSave);

    int lenString = snprintf(status, sizeof(status), "%.20s%s - %d lines", eConfig.filename ? eConfig.filename : "[No Name]", isDirty ? "*" : "", eConfig.numRows);
    int rLen = snprintf(rStatus, sizeof(rStatus), "%d/%d", eConfig.cursorY + 1, eConfig.numRows + 1);
    int cLen = snprintf(cStatus, sizeof(cStatus), "^T - help");

    if (lenString > eConfig.screencols)
        lenString = eConfig.screencols;
    if (cLen > eConfig.screencols)
        cLen = eConfig.screencols;
    if (rLen > eConfig.screencols)
        rLen = eConfig.screencols;

    abAppend(appendBuffer, status, lenString);

    int centerStartCol = (eConfig.screencols - cLen) / 2;

    if (lenString < centerStartCol)
    {
        while (lenString < centerStartCol)
        {
            abAppend(appendBuffer, " ", 1);
            lenString++;
        }
        abAppend(appendBuffer, cStatus, cLen);
        lenString += cLen;
    }

    int rightStartCol = eConfig.screencols - rLen;

    if (lenString < rightStartCol)
    {
        while (lenString < rightStartCol)
        {
            abAppend(appendBuffer, " ", 1);
            lenString++;
        }
        abAppend(appendBuffer, rStatus, rLen);
        lenString += rLen;
    }

    while (lenString < eConfig.screencols)
    {
        abAppend(appendBuffer, " ", 1);
        lenString++;
    }

    abAppend(appendBuffer, "\x1b[m", 4);
}

void editorDrawnMessageBar(abuf *appendBuffer)
{
    abAppend(appendBuffer, "\r\n", 2);
    abAppend(appendBuffer, "\x1b[K", 3);

    int msgLen = strlen(eConfig.statusMessage);
    if (msgLen > eConfig.screencols)
        msgLen = eConfig.screencols;

    if (msgLen)
    {
        abAppend(appendBuffer, eConfig.statusMessage, msgLen);
    }
}

void editorDrawnHelpBar(abuf *appendBuffer, char arr[][20], int numItems, int padding, int boxOfDraw)
{
    int drawingCols = 0;

    for (int count = 0; count < numItems; count++)
    {
        int blockStart = (boxOfDraw * count) + padding;

        while (drawingCols < blockStart && drawingCols < eConfig.screencols)
        {
            abAppend(appendBuffer, " ", 1);
            drawingCols++;
        }

        int lenString = strlen(arr[count]);

        if (drawingCols + lenString <= eConfig.screencols)
        {
            abAppend(appendBuffer, arr[count], strlen(arr[count]));
            drawingCols += lenString;
        }
    }

    while (drawingCols < eConfig.screencols)
    {
        abAppend(appendBuffer, " ", 1);
        drawingCols++;
    }
}

void editorRefreshScreen()
{
    int isMessageActive = eConfig.statusMessage[0] != '\0' && (time(NULL) - eConfig.statusMsgTime < 5);
    int isHelpBarActive = eConfig.isComandBar;

    getWindowSize(&eConfig.screenrows, &eConfig.screencols);
    eConfig.screenrows -= 1;

    if (isMessageActive)
        eConfig.screenrows -= 1;
    if (isHelpBarActive)
        eConfig.screenrows -= 1;

    editorScroll();

    abuf appendBuffer = ABUF_INIT;

    abAppend(&appendBuffer, CURSOR_INVISIBLE, 6);
    abAppend(&appendBuffer, "\x1b[H", 3);

    editorDrawRows(&appendBuffer);
    editorDrawnStatusBar(&appendBuffer);

    if (isMessageActive)
    {
        editorDrawnMessageBar(&appendBuffer);
    }

    if (isHelpBarActive)
    {
        int numCmds = sizeof(eConfig.comandBar) / sizeof(eConfig.comandBar[0]);
        editorDrawnHelpBar(&appendBuffer, eConfig.comandBar, numCmds, 5, 15);
    }

    char buf[32];
    snprintf(buf, sizeof(buf), "\x1b[%d;%dH", (eConfig.cursorY - eConfig.rowOff) + 1, (eConfig.renderX - eConfig.colOff) + 1);
    abAppend(&appendBuffer, buf, strlen(buf));

    abAppend(&appendBuffer, CURSOR_VISIBLE, 6);

    write(STDOUT_FILENO, appendBuffer.buffer, appendBuffer.len);
    abFree(&appendBuffer);
}

void editorSetStatusMessage(const char *format, ...)
{
    va_list ap;

    va_start(ap, format);
    vsnprintf(eConfig.statusMessage, sizeof(eConfig.statusMessage), format, ap);

    va_end(ap);

    eConfig.statusMsgTime = time(NULL);
}