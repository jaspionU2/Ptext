#define _DEFAULT_SOURCE
#define _BSD_SOURCE
#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "editor.h"
#include "terminal.h"

int editorRowCxToRx(EditorRow *row, int cursorX)
{
    int renderX = 0;
    for (int i = 0; i < cursorX; i++)
    {
        if (row->chars[i] == '\t')
            renderX += (PTEXT_TAB_STOP - 1) - (renderX % PTEXT_TAB_STOP);
        renderX++;
    }
    return renderX;
}

void editorUpdateRow(EditorRow *row)
{
    int tabs = 0;

    for (int i = 0; i < row->size; i++)
    {
        if (row->chars[i] == '\t')
            tabs++;
    }

    free(row->render);
    row->render = malloc(row->size + tabs * (PTEXT_TAB_STOP - 1) + 1);

    int index = 0;
    for (int i = 0; i < row->size; i++)
    {
        if (row->chars[i] == '\t')
        {
            row->render[index++] = ' ';
            while (index % PTEXT_TAB_STOP != 0)
                row->render[index++] = ' ';
        }
        else
        {
            row->render[index++] = row->chars[i];
        }
    }
    row->render[index] = '\0';
    row->rsize = index;
}

void editorAppendRow(char *s, size_t len)
{
    eConfig.row = realloc(eConfig.row, sizeof(EditorRow) * (eConfig.numRows + 1));

    int at = eConfig.numRows;
    eConfig.row[at].size = len;
    eConfig.row[at].chars = malloc(len + 1);
    memcpy(eConfig.row[at].chars, s, len);
    eConfig.row[at].chars[len] = '\0';

    eConfig.row[at].rsize = 0;
    eConfig.row[at].render = NULL;

    eConfig.numRows++;
    editorUpdateRow(&eConfig.row[at]);
}

void editorInsertNewline()
{
    int atCol = eConfig.cursorX;
    int atRow = eConfig.cursorY;

    editorAppendRow("", 0);

    EditorRow *row = &eConfig.row[atRow];

    if (eConfig.numRows == 1)
    {
        row->chars = malloc(1);
        row->chars = '\0';
        row->size = 0;
        row->render = NULL;
        row->rsize = 0;
        editorUpdateRow(row);

        editorAppendRow("", 0);
        EditorRow *nextRow = &eConfig.row[atRow + 1];
        nextRow->chars = malloc(1);
        nextRow->chars[0] = '\0';
        nextRow->size = 0;
        nextRow->render = NULL;
        nextRow->rsize = 0;
        editorUpdateRow(nextRow);
        return;
    }

    if (atCol < 0 || atCol > row->size)
        atCol = row->size;

    if (atRow < eConfig.numRows - 1)
    {
        memmove(&eConfig.row[atRow + 1], row, sizeof(EditorRow) * (eConfig.numRows - 1 - atRow));
    }

    EditorRow *newRow = &eConfig.row[atRow + 1];
    newRow->chars = NULL;
    newRow->size = 0;
    newRow->render = NULL;
    newRow->rsize = 0;

    if (atCol < row->size)
    {
        char *newText = (char *)malloc((row->size - atCol) + 1);
        char *oldText = (char *)malloc(atCol + 1);

        if (!newText || !oldText)
            die("malloc");

        memcpy(newText, &row->chars[atCol], row->size - atCol);
        memcpy(oldText, row->chars, atCol);

        newText[row->size - atCol] = '\0';
        oldText[atCol] = '\0';

        newRow->chars = newText;
        newRow->size = row->size - atCol;

        row->chars = oldText;
        row->size = atCol;

        editorUpdateRow(row);
        editorUpdateRow(newRow);
    }
    else
    {
        newRow->chars = malloc(1);
        newRow->chars[0] = '\0';
        newRow->size = 0;

        editorUpdateRow(newRow);
    }
}

void editorDelChar(int atRow, int atCol)
{
    if (eConfig.numRows <= 0 || atRow < 0 || atRow >= eConfig.numRows)
        return;

    EditorRow *row = &eConfig.row[atRow];

    if (atCol < 0 || atCol >= row->size)
        return;

    memmove(&row->chars[atCol], &row->chars[atCol + 1], row->size - atCol - 1);

    row->size -= 1;
    row->chars[row->size] = '\0';

    editorUpdateRow(row);
}

void editorBackspaceChar(int atRow, int atCol)
{
    if (eConfig.numRows <= 0 || atRow < 0 || atRow >= eConfig.numRows)
        return;

    EditorRow *row = &eConfig.row[atRow];

    if (atRow == 0 && atCol == 0)
        return;

    if (atCol == 0)
    {
        EditorRow *prevRow = &eConfig.row[atRow - 1];

        size_t combinedSize = row->size + prevRow->size + 1;
        char *buffer = (char *)malloc(combinedSize);

        if (!buffer)
            die("malloc");

        int len = snprintf(buffer, combinedSize, "%s%s", prevRow->chars, row->chars);

        if ((size_t)len >= combinedSize || len < 0)
            die("snprintf");

        buffer[len] = '\0';

        free(prevRow->chars);

        int prevSize = prevRow->size;

        prevRow->chars = buffer;
        prevRow->size = len;
        editorUpdateRow(prevRow);

        free(row->chars);
        free(row->render);

        memmove(row, &eConfig.row[atRow + 1], sizeof(EditorRow) * (eConfig.numRows - atRow - 1));

        eConfig.numRows--;

        EditorRow *tempRow = realloc(eConfig.row, sizeof(EditorRow) * eConfig.numRows);

        if (!tempRow)
            die("realloc");

        eConfig.row = tempRow;

        eConfig.cursorY = atRow - 1;
        eConfig.cursorX = prevSize;
    }
    else
    {
        memmove(&row->chars[atCol - 1], &row->chars[atCol], row->size - atCol);

        row->size -= 1;
        row->chars[row->size] = '\0';

        editorUpdateRow(row);

        eConfig.cursorX = atCol - 1;
    }
}

void editorInsertInRow(EditorRow *atRow, int atCol, int key)
{
    if (atCol < 0 || atCol > atRow->size)
        atCol = atRow->size;

    char *temp = realloc(atRow->chars, atRow->size + 2);
    if (!temp)
        die("realloc");

    atRow->chars = temp;

    if (atCol < atRow->size)
    {
        for (int i = atRow->size; i >= atCol; i--)
        {
            atRow->chars[i + 1] = atRow->chars[i];
        }
    }

    atRow->chars[atCol] = key;
    atRow->size++;

    if (atCol == atRow->size - 1)
        atRow->chars[atCol + 1] = '\0';

    editorUpdateRow(atRow);
}

void editorInsertChar(int c)
{
    if (eConfig.cursorY == eConfig.numRows)
        editorAppendRow("", 0);

    editorInsertInRow(&eConfig.row[eConfig.cursorY], eConfig.cursorX, c);
}