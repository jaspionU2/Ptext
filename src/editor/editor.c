#define _DEFAULT_SOURCE
#define _BSD_SOURCE
#define _GNU_SOURCE

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

#include "editor.h"
#include "terminal.h"

EditorConfig eConfig;
int drawMessageBar = 0;

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

void editorOpen(char *filename)
{
    free(eConfig.filename);
    eConfig.filename = strdup(filename);

    FILE *fp = fopen(filename, "r");
    if (!fp)
        die("fopen");

    char *line = NULL;
    size_t lineCap = 0;
    ssize_t lineLen;
    while ((lineLen = getline(&line, &lineCap, fp)) != -1)
    {
        while (lineLen > 0 && (line[lineLen - 1] == '\n' || line[lineLen - 1] == '\r'))
            lineLen--;

        editorAppendRow(line, lineLen);
    }
    free(line);
    fclose(fp);
}

char *editorRowToString(int *buflen)
{
    int strLen = 0;
    char *buffer;

    int nrows = eConfig.numRows;
    for (int i = 0; i < nrows; i++)
    {
        strLen += eConfig.row[i].size + 1;
    }

    buffer = malloc(strLen + 1);

    if (!buffer)
        die("malloc");

    *buflen = strLen;

    char *ptrBuffer = buffer;

    for (int i = 0; i < nrows; i++)
    {
        memcpy(ptrBuffer, eConfig.row[i].chars, eConfig.row[i].size);
        ptrBuffer += eConfig.row[i].size;
        *ptrBuffer = '\n';
        ptrBuffer++;
    }
    *ptrBuffer = '\0';

    return buffer;
}

void editorSave()
{
    if (!eConfig.filename)
        return;

    int buflen;
    char *buffer = editorRowToString(&buflen);

    int fd = open(eConfig.filename, O_RDWR | O_CREAT, 0644);

    if (fd != -1)
    {
        if (ftruncate(fd, buflen) != -1)
        {
            if (write(fd, buffer, buflen) == buflen)
            {
                close(fd);
                free(buffer);
                editorSetStatusMessage("%d bytes written to disk", buflen);
                return;
            }
        }
        close(fd);
    }

    free(buffer);
    editorSetStatusMessage("Can't save! I/O error: %s", strerror(errno));
}

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
    char editorActionType[20];
    int deletedChar;

    switch (c)
    {
    case CTRL_KEY('q'):
        clearScreen('2');
        moveCursor(0, 0);
        exit(0);
        break;

    case CTRL_KEY('s'):
        editorSave();
        break;

    case CTRL_KEY('t'):
        eConfig.isComandBar = eConfig.isComandBar ? 0 : 1;
        break;

    case CTRL_KEY('z'):
        editorUndo();
        break;

    case (int)'\r':
    case (int)'\n':
        eConfig.activeCommand.len = eConfig.row[eConfig.cursorY].size;
        eConfig.activeCommand.actualPosition = eConfig.cursorX;
        editorInsertNewline();
        editorMoveCursor(ARROW_DOWN);
        eConfig.cursorX = 0;
        strcpy(editorActionType, "newline");
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
            editorDelChar(eConfig.cursorY, eConfig.cursorX);
            strcpy(editorActionType, "delete");
        }    
        else if (c == BACKSPACE || c == 8)
        {
            deletedChar = eConfig.row[eConfig.cursorY].chars[eConfig.cursorX - 1];
            editorBackspaceChar(eConfig.cursorY, eConfig.cursorX);
            strcpy(editorActionType,  "backspace");
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
        editorInsertChar(c);
        editorMoveCursor(ARROW_RIGHT);
        strcpy(editorActionType, "add");
        break;
    }

    if (((c >= CTRL_KEY('a') && c <= CTRL_KEY('z')) && c != '\r' && c != '\n') || (c >= ARROW_LEFT && c <= PAGE_DOWN)) 
        return;

    if (c == DEL_KEY || c == BACKSPACE || c == CTRL_KEY('h'))
        c = deletedChar;

    editorBackup(editorActionType, c);
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

void editorBackup(char *actionType, int command)
{
    CommandHistory *currentHistory = &eConfig.activeCommand;

    int shouldFlush = 0;
    if (currentHistory->command != NULL)
    {
        if (strcmp(currentHistory->typeAction, actionType) != 0)
            shouldFlush = 1;
        else if (strcmp(actionType, "add") == 0 && currentHistory->actualPosition + 1 != eConfig.cursorX)
            shouldFlush = 1;
        else if (strcmp(actionType, "delete") == 0 || (strcmp(actionType, "backspace")) == 0)
        {
            int cursorCompensado = eConfig.cursorX;
            if (eConfig.cursorX >= currentHistory->startX)
            {
                cursorCompensado += currentHistory->len;
            }

            if (abs(currentHistory->actualPosition - cursorCompensado) > 1)
            {
                shouldFlush = 1;
            }
        }
        else if (strcmp(actionType, "newline") == 0)
        {
            shouldFlush = 1;
        }
        else if (currentHistory->row != eConfig.cursorY)
        {
            shouldFlush = 1;
        }
    }

    if (shouldFlush)
    {
        int historyLen = snprintf(NULL, 0, "%s;%d;%d;%s", currentHistory->typeAction, currentHistory->row, currentHistory->startX, currentHistory->command);

        if (historyLen < 0)
            die("snprintf");

        char *historyEntry = malloc(historyLen + 1);

        if (!historyEntry)
            die("malloc");

        snprintf(historyEntry, historyLen + 1, "%s;%d;%d;%s", currentHistory->typeAction, currentHistory->row, currentHistory->startX, currentHistory->command);

        push(&eConfig.undoStack, historyEntry);

        free(currentHistory->typeAction);
        free(currentHistory->command);

        currentHistory->command = NULL;
    }

    if (currentHistory->command == NULL)
    {
        currentHistory->typeAction = strdup(actionType);

        if (!currentHistory->typeAction)
            die("strdup");

        size_t newSize = 1;
        size_t startIndex = currentHistory->actualPosition;

        if (strcmp(actionType, "newline") == 0 && startIndex < currentHistory->len)
            newSize = currentHistory->len - startIndex;

        currentHistory->command = malloc(newSize + 1);

        if (!currentHistory->command)
            die("malloc");

        if (strcmp(actionType, "newline") == 0 && startIndex < currentHistory->len)
            memcpy(currentHistory->command, eConfig.row[eConfig.cursorY].chars, newSize);
        else
            currentHistory->command[0] = (char)command;

        currentHistory->command[newSize] = '\0';
        currentHistory->actualPosition = eConfig.cursorX;
        currentHistory->row = eConfig.cursorY;
        currentHistory->startX = eConfig.cursorX;
        currentHistory->len = newSize;
        return;
    }

    size_t currentCommandLen = currentHistory->len;
    char *resizedCommand = realloc(currentHistory->command, currentCommandLen + 2);

    if (!resizedCommand)
        die("realloc");

    currentHistory->command = resizedCommand;

    if (strcmp(actionType, "delete") == 0 || strcmp(actionType,  "backspace") == 0)
    {
        if (eConfig.cursorX < currentHistory->actualPosition)
        {
            memmove(&currentHistory->command[1], &currentHistory->command[0], currentCommandLen);
            currentHistory->command[0] = (char)command;
        }
        else
        {
            currentHistory->command[currentCommandLen] = (char)command;
        }
    }
    else 
        currentHistory->command[currentCommandLen] = (char)command;

    currentHistory->command[currentCommandLen + 1] = '\0';
    currentHistory->actualPosition = eConfig.cursorX;
    currentHistory->len++;
}

void editorUndo()
{
    CommandHistory *currentHistory = &eConfig.activeCommand;

    if (currentHistory->command != NULL && currentHistory->typeAction != NULL)
    {
        int historyLen = snprintf(NULL, 0, "%s;%d;%d;%s",
                                  currentHistory->typeAction,
                                  currentHistory->row,
                                  currentHistory->startX,
                                  currentHistory->command);

        if (historyLen < 0)
            die("snprintf");

        char *historyEntry = malloc((size_t)historyLen + 1);
        if (!historyEntry)
            die("malloc");

        snprintf(historyEntry, (size_t)historyLen + 1, "%s;%d;%d;%s",
                 currentHistory->typeAction,
                 currentHistory->row,
                 currentHistory->startX,
                 currentHistory->command);

        push(&eConfig.undoStack, historyEntry);

        free(currentHistory->typeAction);
        free(currentHistory->command);
        currentHistory->typeAction = NULL;
        currentHistory->command = NULL;
        currentHistory->len = 0;
    }

    if (!eConfig.undoStack)
        return;

    char *action = pop(&eConfig.undoStack);

    if (!action)
        return;

    char *actionCopy = strdup(action);

    if (!actionCopy)
        die("strdup");

    char *pieces[4];

    char *token = strtok(actionCopy, ";");

    if (!token)
        die("strtok");

    int i = 0;
    for (; token != NULL && i < 4; i++)
    {
        pieces[i] = token;
        token = strtok(NULL, ";");
    }

    if (i < 4)
        die("strtok");

    char *typeAction = strdup(pieces[0]);
    int row = atoi(pieces[1]);
    int startCol = atoi(pieces[2]);
    size_t commandLen = strlen(pieces[3]);

    if (strcmp(typeAction, "add") == 0)
    {
        eConfig.cursorX = startCol + commandLen - 1;

        for (size_t i = 0; i < commandLen; i++)
        {
            editorBackspaceChar(row, eConfig.cursorX);
        }
    }
    else if (strcmp(typeAction, "backspace") == 0 || strcmp(typeAction, "delete") == 0)
    {
        if (eConfig.cursorX != startCol)
            eConfig.cursorX = startCol;

        for (size_t i = 0; i < commandLen; i++)
        {
            editorInsertChar(pieces[3][i]);
            editorMoveCursor(ARROW_RIGHT);
        }
    }
    else
    {
        eConfig.cursorX = 0;
        editorBackspaceChar(row, startCol);
    }

    if (strcmp(typeAction, "delete") == 0)
        eConfig.cursorX = startCol;

    push(&eConfig.redoStack, action);

    free(typeAction);
    free(action);
}

void editorRedo()
{
    if (!eConfig.redoStack)
        return;

    char *action = pop(&eConfig.redoStack);

    if (!action)
        return;

    push(&eConfig.undoStack, action);

    free(action);
}

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
    int lenString = snprintf(status, sizeof(status), "%.20s  - %d lines", eConfig.filename ? eConfig.filename : "[No Name]", eConfig.numRows);
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

void editorSetHelpComandBar(char *arr[])
{
    int comandBarSize = sizeof(eConfig.comandBar) / sizeof(eConfig.comandBar[0]);
    int maxStringSize = sizeof(eConfig.comandBar[0]);
    int index = 0;

    while (arr[index] != NULL && index < comandBarSize)
    {
        strncpy(eConfig.comandBar[index], arr[index], maxStringSize - 1);

        eConfig.comandBar[index][maxStringSize - 1] = '\0';

        index++;
    }
}

void initEditor()
{
    eConfig.cursorX = 0;
    eConfig.cursorY = 0;
    eConfig.renderX = 0;
    eConfig.rowOff = 0;
    eConfig.colOff = 0;
    eConfig.numRows = 0;
    eConfig.row = NULL;
    eConfig.filename = NULL;
    eConfig.statusMessage[0] = '\0';
    eConfig.statusMsgTime = 0;
    eConfig.isComandBar = 0;
    eConfig.undoStack = NULL;
    eConfig.redoStack = NULL;
    eConfig.lastSave = NULL;

    char *comands[] = {
        "^Q Quit",
        "^S Save",
        "^T Help",
        "^Z Undo",
        "^Y Redo",
        NULL};

    editorSetHelpComandBar(comands);

    if (getWindowSize(&eConfig.screenrows, &eConfig.screencols) == -1)
        die("getWindowSize");

    eConfig.screenrows -= 1;
}
