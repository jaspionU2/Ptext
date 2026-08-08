#define _DEFAULT_SOURCE
#define _BSD_SOURCE
#define _GNU_SOURCE

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>

#include "editor.h"
#include "terminal.h"

void editorNewFile(char *filename)
{
    char cwd[1024];

    if (getcwd(cwd, sizeof(cwd)) == NULL)
    {
        die("getcwd");
    }

    char filenamePath[200];

    int result = snprintf(filenamePath, sizeof(filenamePath), "%s/%s", cwd, filename);

    if (result < 0 || (size_t)result >= sizeof(filenamePath))
    {
        die("snprintf(). Error formating new file path.");
    }

    FILE *fp = fopen(filenamePath, "a");

    if (!fp)
    {
        die("Error opening or creating file!");
    }

    eConfig.filename = strdup(filenamePath);
    fclose(fp);
}

void editorOpen(char *filename)
{
    free(eConfig.filename);
    eConfig.filename = strdup(filename);

    struct stat buffer;
    if (stat(filename, &buffer) != 0)
    {
        editorNewFile(filename);
    }
    else
    {
        if (S_ISDIR(buffer.st_mode))
        {
            editorNewFile(filename);
        }
    }

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
    {
        editorNewFile("new_file.txt");
    }

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

                if (eConfig.activeCommand.command)
                {
                    editorHistoryToUndo(&eConfig.activeCommand);
                }

                eConfig.lastSave = eConfig.undoStack;
                eConfig.quitConfirm = 0;
                return;
            }
        }
        close(fd);
    }

    free(buffer);
    editorSetStatusMessage("Can't save! I/O error: %s", strerror(errno));
}