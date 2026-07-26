#ifndef STACK_H
#define STACK_H

typedef struct stack
{
    char *info;
    struct stack *last;
} stack;

stack *newNode(char *info);
char *pop(stack **top);
void push(stack **top, char *info);
void freeStack(stack **top);

#endif