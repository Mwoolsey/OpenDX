/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>


/*
 * Header: /usr/people/gresh/code/svs/src/libdx/RCS/seglist.c,v 5.2 93/01/13 17:58:38 gda Exp Locker: gresh 
 * 
 */

#include <stdio.h>
#include <dx/dx.h>

SegList *
DXNewSegList(int sz, int n, int m)
{
    SegList *slist = NULL;

    slist = (SegList *)DXAllocate(sizeof(SegList));
    if (! slist)
	goto error;

    slist->incAlloc      = m;
    slist->nextItem      = 0;
    slist->itemSize      = sz;

    if (n > 0)
    {
	slist->segments = (SegListSegment *)DXAllocate(sizeof(SegListSegment));
	if (! slist->segments)
	    goto error;
    
	slist->segments->items = (char *)DXAllocate(n*sz);
	if (! slist->segments->items)
	    goto error;
    
	slist->segments->next = NULL;
	slist->segments->size = n;
	slist->segments->current = 0;

	slist->currentSeg = slist->segments;
    }
    else
    {
	slist->segments = NULL;
	slist->currentSeg = NULL;
    }

    return slist;

error:
    if (slist)
    {
	if (slist->segments != NULL)
	    DXFree((Pointer)(slist->segments->items));
	DXFree((Pointer)(slist->segments));
	DXFree((Pointer)(slist));
    }

    return NULL;
}

SegListSegment *
DXNewSegListSegment(SegList *slist, int nItems)
{
    SegListSegment *tmp = (SegListSegment *)DXAllocate(sizeof(SegListSegment));
    if (! tmp)
	goto error;
    
    tmp->items = (char *)DXAllocate(nItems*slist->itemSize);
    if (! tmp->items)
	goto error;
    
    tmp->size = nItems;
    tmp->current = nItems;

    tmp->next = NULL;
    
    if (slist->segments == NULL)
    {
	slist->segments = tmp;
	slist->currentSeg = tmp;
    }
    else
    {
	slist->currentSeg->next = tmp;
	slist->currentSeg = tmp;
    }

    slist->nextItem += nItems;

    return tmp;

error:
    if (tmp)
	DXFree((Pointer)(tmp->items));
    DXFree((Pointer)tmp);
    return NULL;
}

Pointer
DXNewSegListItem(SegList *slist)
{
    SegListSegment *tmp = NULL;

    if (slist->currentSeg->current == slist->currentSeg->size)
    {
	if (slist->incAlloc <= 0)
	{
	    DXSetError(ERROR_NO_MEMORY, "SegList cannot be appended");
	    return NULL;
	}
	    
	tmp = (SegListSegment *)DXAllocate(sizeof(SegListSegment));
	if (! tmp)
	    goto error;
	
	tmp->items = (char *)DXAllocate(slist->incAlloc*slist->itemSize);
	if (! tmp->items)
	    goto error;
	
	tmp->size = slist->incAlloc;
	tmp->current = 0;
    
	tmp->next = NULL;

	if (slist->segments == NULL)
	{
	    slist->segments = tmp;
	    slist->currentSeg = tmp;
	}
	else
	{
	    slist->currentSeg->next = tmp;
	    slist->currentSeg = tmp;
	}

	tmp = NULL;
    }

    slist->nextItem ++;
    return (Pointer)(slist->currentSeg->items + 
		(slist->currentSeg->current++ * slist->itemSize));

error:
    if (tmp)
    {
	DXFree((Pointer)tmp->items);
	DXFree((Pointer)tmp);
    }

    return NULL;
}

Error
DXDeleteSegList(SegList *slist)
{
    SegListSegment *next;
    SegListSegment *last;

    if (! slist)
	return OK;

    next = slist->segments;

    while (next)
    {
	last = next;
	next = next->next;

	DXFree((Pointer)last->items);
	DXFree((Pointer)last);
    }

    DXFree((Pointer)slist);

    return OK;
}

Error
DXInitGetNextSegListItem(SegList *slist)
{
    slist->iterSeg = slist->segments;
    slist->next = 0;

    return OK;
}

Pointer
DXGetNextSegListItem(SegList *slist)
{
    if (slist->iterSeg == NULL)
	return NULL;

    if (slist->next == slist->iterSeg->current)
    {
	slist->next = 0;
	slist->iterSeg = slist->iterSeg->next;
	return DXGetNextSegListItem(slist);
    }
    else
	return (Pointer)(slist->iterSeg->items +
				(slist->next++ * slist->itemSize));
}

int 
DXGetSegListItemCount(SegList *slist)
{
    return slist->nextItem;
}


Error
DXInitGetNextSegListSegment(SegList *slist)
{
    slist->iterSeg = slist->segments;
    return OK;
}

SegListSegment *
DXGetNextSegListSegment(SegList *slist)
{
    SegListSegment *slistseg;

    if (slist->iterSeg == NULL)
	return NULL;

    slistseg = slist->iterSeg;
    slist->iterSeg = slistseg->next;

    return slistseg;
}

Pointer
DXGetSegListSegmentPointer(SegListSegment *slistseg)
{
    return (Pointer)slistseg->items;
}

int
DXGetSegListSegmentItemCount(SegListSegment *slistseg)
{
    return slistseg->current;
}
