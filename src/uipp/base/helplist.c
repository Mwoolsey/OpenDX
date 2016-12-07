/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"


#include "helplist.h"
#include <stdio.h>
#include <string.h>
#include <X11/Intrinsic.h>

#define STRCMP(a,b)  ((a) ? ((b) ? strcmp(a,b) : strcmp(a,"")) : \
			    ((b) ? strcmp("",b) : 0))

SpotList *NewList()
{
  SpotList *list;

  list = (SpotList*)XtMalloc(sizeof(SpotList));

  list->head = NULL;
  list->tail = NULL;
  list->current = NULL;
  list->length = 0;
  return(list);
}


void InsertList(SpotList *list, ListNodeType *NewNode)
{

  if (list == NULL || NewNode == NULL)
    printf("Error in history list\n");

  if (list->head == NULL) {
      NewNode->next = NULL;
      NewNode->prev = NULL;
      list->head = NewNode;
      list->tail = NewNode;
      list->current = NewNode;
  } else {
      list->tail->next = NewNode;
      NewNode->next = NULL;
      NewNode->prev = list->tail;
      list->tail = NewNode;
  }
  list->length++;
}

void DeleteLastListNode(SpotList *list)
{
  if (list == NULL)
    printf("Error in history list\n");
  if (list->head == NULL)
    return;
  list->current = list->tail->prev;
  if (list->tail->refname != NULL)
    XtFree(list->tail->refname);
  list->tail->refname = NULL;
  XtFree((char *)list->tail);
  list->tail = list->current;
  if (list->tail)
      list->tail->next = NULL;
  else
      list->head = NULL;
 list->length--;
}

int SearchList(SpotList *list, char *str)
{
  ListNodeType *ref;
  int      index = 0;

  if (list == NULL)
    printf("Error in list\n");

  ref = list->head;

  while (ref != NULL) {
    if (STRCMP(ref->refname,str) == 0)
     return(ref->offset);
    index++;
    ref = ref->next;
  }
  return(0);  /* node not found */
}

void FreeSpotList(SpotList *list)
{
 int i;

 if (list == NULL)
    return;

 for (i = 0; i < list->length; i++)
     DeleteLastListNode(list);
}
