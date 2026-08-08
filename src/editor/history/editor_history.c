#define _DEFAULT_SOURCE
#define _BSD_SOURCE
#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "editor.h"
#include "terminal.h"

void editorHistoryToUndo(CommandHistory *h)
{
    int historyLen = snprintf(NULL, 0, "%s\x1F%d\x1F%d\x1F%s",
                              h->typeAction, h->row, h->startX, h->command);
    if (historyLen < 0)
        die("snprintf");

    char *historyEntry = malloc((size_t)historyLen + 1);
    if (!historyEntry)
        die("malloc");

    snprintf(historyEntry, (size_t)historyLen + 1, "%s\x1F%d\x1F%d\x1F%s",
             h->typeAction, h->row, h->startX, h->command);

    push(&eConfig.undoStack, historyEntry);

    free(h->typeAction);
    free(h->command);
    h->typeAction = NULL;
    h->command = NULL;
    h->len = 0;
    h->row = -1;
    h->startX = -1;
}

void editorBackup(char *actionType, int command)
{
    if (eConfig.redoStack)
    {
        freeStack(&eConfig.redoStack);

        stack *temp = eConfig.redoStack;
        while (temp != NULL)
        {
            if (eConfig.lastSave == temp)
            {
                eConfig.lastSave = NULL;
                break;
            }
            temp = temp->last;
        }
    }

    CommandHistory *currentHistory = &eConfig.activeCommand;

    int shouldFlush = 0;
    if (currentHistory->command != NULL)
    {
        if (strcmp(currentHistory->typeAction, actionType) != 0)
            shouldFlush = 1;
        else if (currentHistory->row != eConfig.cursorY)
            shouldFlush = 1;
        else if (strcmp(actionType, "newline") == 0)
            shouldFlush = 1;
        else if (strcmp(actionType, "mergeLine") == 0)
            shouldFlush = 1;
        else if (strcmp(actionType, "add") == 0 && currentHistory->actualPosition + 1 != eConfig.cursorX)
            shouldFlush = 1;
        else if (strcmp(actionType, "delete") == 0 || strcmp(actionType, "backspace") == 0)
        {
            int diffDeletedPos;

            if (strcmp(actionType, "backspace") == 0)
                diffDeletedPos = currentHistory->actualPosition - (eConfig.cursorX - 1);
            else
                diffDeletedPos = currentHistory->actualPosition - eConfig.cursorX;

            if (diffDeletedPos != 0 && diffDeletedPos != 1)
            {
                shouldFlush = 1;
            }
        }
    }

    if (shouldFlush)
    {
        editorHistoryToUndo(currentHistory);
    }

    if (currentHistory->command == NULL)
    {
        currentHistory->typeAction = strdup(actionType);

        if (!currentHistory->typeAction)
            die("strdup");

        currentHistory->command = malloc(2);

        if (!currentHistory->command)
            die("malloc");

        currentHistory->command[0] = command == -1 ? '\r' : (char)command;
        currentHistory->command[1] = '\0';
        currentHistory->actualPosition = eConfig.cursorX;
        currentHistory->row = eConfig.cursorY;
        currentHistory->startX = eConfig.cursorX;
        currentHistory->len = 1;

        if (strcmp(actionType, "mergeLine") == 0)
        {
            EditorRow lastRow = eConfig.row[eConfig.cursorY - 1];

            currentHistory->row = eConfig.cursorY - 1;
            currentHistory->startX = lastRow.size;
            currentHistory->actualPosition = lastRow.size + 1;
        }
        else if (strcmp(actionType, "backspace") == 0)
        {
            currentHistory->startX = eConfig.cursorX - 1;
            currentHistory->actualPosition = eConfig.cursorX - 1;
        }

        return;
    }

    size_t currentCommandLen = currentHistory->len;
    char *resizedCommand = realloc(currentHistory->command, currentCommandLen + 2);

    if (!resizedCommand)
        die("realloc");

    currentHistory->command = resizedCommand;

    if (strcmp(actionType, "delete") == 0 || strcmp(actionType, "backspace") == 0)
    {
        int posCursorX = strcmp(actionType, "backspace") == 0 ? eConfig.cursorX - 1 : eConfig.cursorX;

        if (posCursorX < currentHistory->actualPosition)
        {
            memmove(&currentHistory->command[1], &currentHistory->command[0], currentCommandLen);
            currentHistory->command[0] = (char)command;

            currentHistory->startX = posCursorX;
        }
        else
        {
            currentHistory->command[currentCommandLen] = (char)command;
        }
    }
    else
        currentHistory->command[currentCommandLen] = (char)command;

    currentHistory->command[currentCommandLen + 1] = '\0';
    currentHistory->actualPosition = strcmp(actionType, "backspace") == 0 ? eConfig.cursorX - 1 : eConfig.cursorX;
    currentHistory->len++;
}

void editorUndo()
{
    CommandHistory *currentHistory = &eConfig.activeCommand;

    if (currentHistory->command != NULL && currentHistory->typeAction != NULL)
    {
        editorHistoryToUndo(currentHistory);
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

    char *token = strtok(actionCopy, "\x1F");

    if (!token)
        die("strtok");

    int i = 0;
    for (; token != NULL && i < 4; i++)
    {
        pieces[i] = token;
        token = strtok(NULL, "\x1F");
    }

    if (i < 4)
        die("strtok");

    char *typeAction = strdup(pieces[0]);
    int row = atoi(pieces[1]);
    int startCol = atoi(pieces[2]);
    size_t commandLen = strlen(pieces[3]);

    if (strcmp(typeAction, "add") == 0)
    {
        eConfig.cursorX = startCol + commandLen;

        for (size_t i = 0; i < commandLen; i++)
        {
            editorBackspaceChar(row, eConfig.cursorX);
        }
    }
    else if (strcmp(typeAction, "backspace") == 0 || strcmp(typeAction, "delete") == 0)
    {
        eConfig.cursorY = row;
        eConfig.cursorX = startCol;

        for (size_t i = 0; i < commandLen; i++)
        {
            editorInsertChar(pieces[3][i]);
            editorMoveCursor(ARROW_RIGHT);
        }

        if (strcmp(typeAction, "delete") == 0)
        {
            eConfig.cursorX = startCol;
        }
    }
    else if (strcmp(typeAction, "newline") == 0)
    {
        editorBackspaceChar(row + 1, 0);
    }
    else if (strcmp(typeAction, "mergeLine") == 0)
    {
        eConfig.cursorY = row;
        eConfig.cursorX = startCol;
        editorInsertNewline();
        eConfig.cursorY = row + 1;
        eConfig.cursorX = 0;
    }

    push(&eConfig.redoStack, action);

    free(typeAction);
    free(actionCopy);
    free(action);
}

void editorRedo()
{
    if (!eConfig.redoStack)
        return;

    char *action = pop(&eConfig.redoStack);

    if (!action)
        return;

    char *actionCopy = strdup(action);

    if (!actionCopy)
        die("strdup");

    char *pieces[4];

    char *token = strtok(actionCopy, "\x1F");

    if (!token)
        die("strtok");

    int i = 0;
    for (; token != NULL && i < 4; i++)
    {
        pieces[i] = token;
        token = strtok(NULL, "\x1F");
    }

    if (i < 4)
        die("strtok");

    char *typeAction = strdup(pieces[0]);
    int row = atoi(pieces[1]);
    int startCol = atoi(pieces[2]);
    size_t commandLen = strlen(pieces[3]);

    if (strcmp(typeAction, "add") == 0)
    {
        eConfig.cursorY = row;
        eConfig.cursorX = startCol;

        for (size_t i = 0; i < commandLen; i++)
        {
            editorInsertChar(pieces[3][i]);
            editorMoveCursor(ARROW_RIGHT);
        }
    }
    else if (strcmp(typeAction, "delete") == 0 || strcmp(typeAction, "backspace") == 0)
    {
        eConfig.cursorY = row;
        eConfig.cursorX = startCol + commandLen;

        for (size_t i = 0; i < commandLen; i++)
        {
            editorBackspaceChar(row, eConfig.cursorX);
        }
    }
    else if (strcmp(typeAction, "newline") == 0)
    {
        eConfig.cursorY = row;
        eConfig.cursorX = startCol;
        editorInsertNewline();
        eConfig.cursorY = row + 1;
        eConfig.cursorX = 0;
    }
    else if (strcmp(typeAction, "mergeLine") == 0)
    {
        editorBackspaceChar(row + 1, 0);
        eConfig.cursorY = row;
        eConfig.cursorX = startCol;
    }

    push(&eConfig.undoStack, action);

    free(action);
    free(actionCopy);
    free(typeAction);
}