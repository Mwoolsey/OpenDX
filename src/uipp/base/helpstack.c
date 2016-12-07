/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"

/*---------------------------------------------------------------------*
 |                                stack.c                              |
 |                           -------------------                       |
 |                                                                     |
 *---------------------------------------------------------------------*/

#ifdef OS2
#include <stdlib.h>
#include <types.h>
#endif
#include <stdio.h>
#include <Xm/Xm.h>
#include "help.h"


/*--------------------------------------------------------------------------*
 |                               New Stack                                  |
 *--------------------------------------------------------------------------*/


Stack *NewStack()
{
  Stack *list;

  list = (Stack *) XtMalloc(sizeof(Stack));

  list->top = NULL;
  list->bottom = NULL;
  list->current = NULL;
  list->length = 0;
  return(list);
}



/*--------------------------------------------------------------------------*
 |                                  Top                                     |
 *--------------------------------------------------------------------------*/
char *Top(Stack *stack)
{
 return(stack->top->value);
}

/*--------------------------------------------------------------------------*
 |                                  Pop                                     |
 *--------------------------------------------------------------------------*/

void Pop(Stack *stack)
{
  if (stack == NULL)
    printf("Error in history list\n");
  if (stack->length == 1)
     return;  /* dont let the stack empty */
  if (stack->top == stack->bottom) {   /* last element on stack */
    XtFree(stack->top->value);
    XtFree((char*)stack->top);
    stack->top = NULL;
    stack->bottom = NULL;
    stack->length = 0;
  } 
  else {
    if (stack->top->value != NULL)
       XtFree(stack->top->value);
    stack->current = stack->top;
    stack->top = stack->top->prev;
    XtFree((char*)stack->current);
  }
 stack->length--;
}

/*--------------------------------------------------------------------------*
 |                                  Push                                    |
 *--------------------------------------------------------------------------*/


void Push(Stack *stack, char *value)
{
  EntryType *NewEntry;
  char *val;

  val = (char *) XtMalloc(strlen(value) + 1);
  strcpy(val,value);
  NewEntry = (EntryType *) XtMalloc(sizeof(EntryType));
  NewEntry->value = val;

  if (stack == NULL || NewEntry == NULL)
    printf("Error in history list\n");

  if (stack->top == NULL) {
      NewEntry->next = NULL;
      NewEntry->prev = NULL;
      stack->top = NewEntry;
      stack->bottom = NewEntry;
      stack->current = NewEntry;
  } else {
      stack->top->next = NewEntry;
      NewEntry->next = NULL;
      NewEntry->prev = stack->top;
      stack->top = NewEntry;
  }
  stack->length++;
}

void FreeStack(Stack *stack)
{
int i;

if (stack == NULL)
  return;

  for (i = 0; i < stack->length; i++)
    Pop(stack);
  XtFree((char*)stack->top);    /* pop won't let the stack go empty */
  XtFree((char*)stack);
}

