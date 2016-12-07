/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>


#include "hwDeclarations.h"
#include "hwList.h"

typedef struct listS
{
    Pointer *items;
    Error   (*delete)(Pointer);
    int     max;
    int     end;
}  *listP;

static void __listEmpty(listP);

static Error
_deleteHwList(Pointer arg)
{
  if (arg)
  {
      listP list = (listP)arg;
      __listEmpty(list);
      DXFree(list->items);
      DXFree(list);
  }
  return OK;
}    

listO
_dxf_newList(int initialSize, Error (*delete)(Pointer))
{
  listO lo;
  listP list;

  list = (Pointer)DXAllocateZero(sizeof(struct listS));
  if (! list)
      goto error;

  list->items = (Pointer)DXAllocateZero(initialSize * sizeof(Pointer));
  if (! list->items)
      goto error;

  list->max = initialSize;
  list->end = 0;
  list->delete = delete;

  lo = (listO)_dxf_newHwObject(HW_CLASS_LIST, (Pointer)list, _deleteHwList);
  return lo;

error:
  if (list)
  {
      if (list->items)
	  DXFree(list->items);
      DXFree(list);
  }
  
  return NULL;
}

Error
_dxf_listAppendItem(listO lo, Pointer item)
{
  listP list = (listP)_dxf_getHwObjectData(lo);
  if (! list)
      return ERROR;

  if(list->end == list->max)
    {
      Pointer tmp = DXReAllocate(list->items,list->max*2*sizeof(Pointer));
      if(!tmp)
	return ERROR;
      list->max *= 2;
      list->items = tmp;
    }
  list->items[list->end++] = item;

  return OK;
}  

Error
_dxf_listEmpty(listO lo)
{
  listP list = (listP)_dxf_getHwObjectData(lo);
  __listEmpty(list);
  return OK;
}

static void
__listEmpty(listP list)
{
  if (list)
  {
      if (list->delete)
      {
	  int i;
	  for (i = 0; i < list->end; i++)
	      if (list->items[i])
		  (*list->delete)(list->items[i]);
      }
  }

  list->end = 0;
}    

Error
_dxf_listSize(listO lo)
{
  listP list = (listP)_dxf_getHwObjectData(lo);
  if (list)
      return list->end;
  else
      return 0;
}

Pointer
_dxf_listGetItem(listO lo, int i)
{
  listP list = (listP)_dxf_getHwObjectData(lo);
  if (list)
      if (i<list->end)
	return list->items[i];

  return NULL;
}
