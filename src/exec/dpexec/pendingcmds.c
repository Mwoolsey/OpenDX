/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>

#if defined(HAVE_STRING_H)
#include <string.h>
#endif

#include <dx/dx.h>
#include "pendingcmds.h"

static PendingCmdList *
_dxf_getPendingCmdList()
{
    Private p = (Private)DXGetCacheEntry(PJL_KEY, 0, 0);
    if (! p)
	return NULL;
    else return (PendingCmdList *)DXGetPrivateData(p);
}

void
_dxf_release_PendingCmdList(PendingCmdList *p)
{
    if (p)
	DXDelete(p->exprivate);
}

Error
_dxf_delete_PendingCmdList(Pointer d)
{
    int i;
    PendingCmdList *pjl = (PendingCmdList *)d;
    PendingCmd *p;

    for (i = 0, p = pjl->pendingCmdList;
		i < pjl->pendingCmdListSize; i++, p++)
    {
	DXFree(p->major);
	DXFree(p->minor);
	DXDelete((Object)p->data);

	p->major = NULL;
	p->minor = NULL;
	p->data  = NULL;
    }

    DXFree(pjl->pendingCmdList);
    DXFree(pjl);

    return OK;
}

static PendingCmdList *
_dxf_newPendingCmdList()
{
    Private p;
    PendingCmdList *pjl;

    pjl = DXAllocate(sizeof(PendingCmdList));
    if (! pjl)
	return NULL;
    
    pjl->pendingCmdListSize = 0;
    pjl->pendingCmdListMax  = 10;

    pjl->pendingCmdList = DXAllocate(10*sizeof(PendingCmd));
    
    if (! pjl->pendingCmdList)
    {
	DXFree(pjl);
	return NULL;
    }

    p = DXNewPrivate((Pointer)pjl, _dxf_delete_PendingCmdList);
    if (! p)
    {
	DXFree(pjl->pendingCmdList);
	DXFree(pjl);
	return NULL;
    }

    if (! DXSetCacheEntry((Object)p, CACHE_PERMANENT, PJL_KEY, 0, 0))
    {
	DXDelete((Object)p);
	DXFree(pjl->pendingCmdList);
	DXFree(pjl);
	return NULL;
    }

    pjl->exprivate = DXReference((Object)p);

    return pjl;
}

static Error
_dxf_growPendingCmdList(PendingCmdList *pjl)
{
    int n = pjl->pendingCmdListMax * 2;

    pjl->pendingCmdList = DXReAllocate((Pointer)(pjl->pendingCmdList),
				    n*sizeof(PendingCmd));

    if (! pjl->pendingCmdList)
	return ERROR;

    pjl->pendingCmdListMax  = n;
    
    return OK;
}

Error
DXSetPendingCmd(char *major, char *minor, int (*job)(Private), Private data)
{
    int i;
    PendingCmd *p;
    PendingCmdList *pjl = _dxf_getPendingCmdList();

    if (!pjl){
	if (!job)
	    return OK;
	else
	    if (NULL == (pjl = _dxf_newPendingCmdList()))
		goto error;
    }

    for (i = 0, p = pjl->pendingCmdList;
		i < pjl->pendingCmdListSize; i++, p++)
    {
	/*
	 * Ignore any already-deleted tasks
	 */
	if (p->job)
	{
	    if (!strcmp(p->major, major) &&
		   ((minor == NULL && p->minor == NULL) ||
		   !strcmp(p->minor, minor)))
	    {
		if (job)
		{
		    p->job  = job;
		    p->data = (Private)DXReference((Object)data);
		    goto done;
		}
		else
		{
		    DXFree(p->major);
		    DXFree(p->minor);
		    DXDelete((Object)p->data);
		    p->job = NULL;
		    goto done;
		}
	    }
	}
    }

    if (pjl->pendingCmdListSize == pjl->pendingCmdListMax)
	if (! _dxf_growPendingCmdList(pjl))
	    goto error;

    p = pjl->pendingCmdList + pjl->pendingCmdListSize++;

    p->major = DXAllocate(strlen(major) + 1);
    if (! p->major)
    {
	p->minor = NULL;
	goto error;
    }

    strncpy(p->major, major, strlen(major) + 1);

    if (minor)
    {
	p->minor = DXAllocate(strlen(minor) + 1);
	if (! p->minor)
	    goto error;

	strncpy(p->minor, minor, strlen(minor) + 1);
    }
    else
	p->minor = NULL;

    p->job  = job;
    p->data = data;

done:
    _dxf_release_PendingCmdList(pjl);
    return OK;

error:
    _dxf_release_PendingCmdList(pjl);
    return ERROR;
}

Error
_dxf_RunPendingCmds()
{
    int i, j;
    PendingCmdList *pjl = _dxf_getPendingCmdList();

    if (!pjl)
	goto done;

    for (i = 0; i < pjl->pendingCmdListSize; i++)
	if (! (pjl->pendingCmdList[i].job)(pjl->pendingCmdList[i].data))
	    goto error;
    
    /*
     * Compress it, removing deleted entries
     */
    for (i = j = 0; i < pjl->pendingCmdListSize; i++)
	if (pjl->pendingCmdList[i].job)
	{
	    if (i != j)
		pjl->pendingCmdList[j] = pjl->pendingCmdList[i];
	    j++;
	}
    
    pjl->pendingCmdListSize = j;

done:
    _dxf_release_PendingCmdList(pjl);
    return OK;

error:
    _dxf_release_PendingCmdList(pjl);
    return ERROR;
}

Error
_dxf_CleanupPendingCmdList()
{
    DXSetCacheEntry(NULL, CACHE_PERMANENT, PJL_KEY, 0, 0);
    return OK; 
}
