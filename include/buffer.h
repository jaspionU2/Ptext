#ifndef BUFFER_H
#define BUFFER_H

#include <stddef.h>

typedef struct
{
    char *buffer;
    int len;
} abuf;

#define ABUF_INIT {NULL, 0}

void abAppend(abuf *appendBuffer, const char *string, int len);
void abFree(abuf *appendBuffer);

#endif