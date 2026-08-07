#define _DEFAULT_SOURCE
#define _BSD_SOURCE
#define _GNU_SOURCE

#include <string.h>

#include "editor.h"
#include "terminal.h"

static void editorSetHelpComandBar(char *arr[])
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