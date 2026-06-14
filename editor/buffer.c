#include "buffer.h"

#include <stdlib.h>
#include <string.h>

void abAppend(abuf *appendBuffer, const char *string, int len)
{
    char *newString = realloc(appendBuffer->buffer, appendBuffer->len + len);

    if (newString == NULL)
        return;
    memcpy(&newString[appendBuffer->len], string, len);
    appendBuffer->buffer = newString;
    appendBuffer->len += len;
}

void abFree(abuf *appendBuffer)
{
    free(appendBuffer->buffer);
}
