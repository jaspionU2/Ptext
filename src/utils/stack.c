#include "stack.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

stack *newNode(char *info)
{
    stack *node = (stack *)malloc(sizeof(stack));
    if (!node)
        return NULL;

    node->info = (char *)malloc(strlen(info) + 1);
    if (!node->info)
    {
        free(node);
        return NULL;
    }

    strcpy(node->info, info);
    node->last = NULL;
    return node;
}

char *pop(stack **top)
{
    if (!top || !*top)
        return NULL;

    stack *node = *top;

    char *temp = node->info;

    *top = node->last;
    free(node); 

    return temp; 
}

void push(stack **top, char *info)
{
    stack *node = newNode(info);
    if (!node)
        return; 

    node->last = *top;
    *top = node;
}

void freeStack(stack **top)
{
    if (!top || !*top)
        return;

    stack *temp = *top;
    while (temp != NULL)
    {
        stack *aux = temp->last;
        free(temp->info);
        free(temp);
        temp = aux;
    }
    *top = NULL;
}