/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>


#include <dx/dx.h>
#include "graphqueue.h"
#include "config.h"
#include "pmodflags.h"
#include "utils.h"
#include "log.h"
#include "packet.h"
#include "cache.h"
#include "_variable.h"
#include "evalgraph.h"

typedef struct gq_elem
{
    struct gq_elem	*next;		/* linked list of elements */
    Program		*func;		/* root node of graph */
} gq_elem;

static lock_type	*gq_sem;
static gq_elem		*gq_head;
static gq_elem		*gq_tail;
static gq_elem		**gq_curr;
static int		have_lock = 0;

void	_dxf_ExGQPrint (char *s);

#define	GQ_NULL(_x)	if (_x == NULL) return (ERROR);

/*
 * Initialize the structures and semaphores for the graph queue.
 */
Error _dxf_ExGQInit (int nprocs)
{
    if ((gq_sem = (lock_type *) DXAllocate (sizeof (lock_type))) == NULL)
	return (ERROR);

    if (DXcreate_lock (gq_sem, "graph queue") != OK)
	return (ERROR);

    gq_curr	= (gq_elem **) DXAllocate (sizeof (gq_elem *));

    GQ_NULL (gq_curr);

    gq_head = NULL;
    gq_tail = NULL;

    *gq_curr = NULL;

    return (OK);
}

/*
 * Insert an entry into the graph queue.
 */
void _dxf_ExGQEnqueue (Program *func)
{
    gq_elem	*elem;

    if (func == NULL)
	return;

    if ((elem = (gq_elem *) DXAllocate (sizeof (gq_elem))) == NULL)
	_dxf_ExDie ("_dxf_ExGQEnqueue:  can't DXAllocate");

    elem->func  = func;
    elem->next  = NULL;

    DXlock (gq_sem, 0);

	if (gq_head)
	    (gq_tail)->next = elem;		/* add to end */
	else
	    gq_head = elem;			/* new list */

	gq_tail = elem;

    DXunlock (gq_sem, 0);

    DXDebug ("*1", "[%08x] IN  to gq  %d", func, func->graphId);
}

/*
 * Remove the next entry from the graph queue if there is an available
 * execution slot for it.  Also make note of any node which have the
 * MODULE_SEQUENCED flag set and make sure that two of them don't fire
 * at the same time.
 */

Program *_dxf_ExGQDequeue ()
{
    gq_elem	*elem;
    gq_elem	*tofree = NULL;
    Program	*func;

    /*
     * Quick outs.
     */

    if (gq_head == NULL)
	return (NULL);

    /*
     * Only bother doing this if we have an open graph slot
     */

    if (*gq_curr != NULL && SIZE_LIST(gq_head->func->funcs) != 0)
	return (NULL);

    DXsyncmem ();

    if (! DXtry_lock (gq_sem, 0))
	return (NULL);

    /*
     * Almost as quick outs
     */

    if (((elem = gq_head) == NULL)	||	/* nothing to do	*/
	(*gq_curr != NULL &&
	    SIZE_LIST(gq_head->func->funcs) != 0)) /* someone slipped in */
    {
	DXunlock (gq_sem, 0);
	return (NULL);
    }

    gq_head = elem->next;

    if (gq_tail == elem)			/* last one */
	gq_tail = NULL;

    if (*gq_curr == NULL)
	*gq_curr = elem;
    else
	tofree = elem;

    DXunlock (gq_sem, 0);

    /*
     * This can be outside of the lock since we are the one who set it and
     * everyone else is just reading it.
     */

    func = elem->func;
    if (tofree != 0)
	DXFree ((Pointer)tofree);

    if (func)
	DXDebug ("*1", "[%08x] OUT of gq  %d", func, func->graphId);

    if (SIZE_LIST(func->funcs) == 0) {
	ExMarkTime(8, "enq assign");
    }
    else {
	ExMarkTime(8, "enq module");
    }

    return (func);
}

/* return pointer to currently running program */
Program *_dxf_ExGQCurrent ()
{
    if (*gq_curr == NULL) 
        return (NULL);
    return((*gq_curr)->func);
}

static void ExResetGraphKill ()
{
    *_dxd_exKillGraph = FALSE;
}

/*
 * Called when a graph (program) has completed.  Blow the graph away and
 * note that none is running. Slave will call this with NULL program, 
 * just delete current. 
 */

void _dxf_ExGQDecrement (Program *p)
{
    Program     *delete = NULL;
    Pointer     tofree  = NULL;
    gq_elem     *curr   = NULL;
    int         more    = FALSE;

    DXlock (gq_sem, 0);

    if ((*gq_curr == NULL) || (p && (*gq_curr)->func != p))
    {
	DXunlock (gq_sem, 0);
        if(p)
	{
	    if (p->runable > 0)
		p->deleted = TRUE;
            else {
                /* only the master has to send a complete message to UI */
                if(!_dxd_exRemoteSlave)
                    _dxf_ExSPack(PACK_COMPLETE, p->graphId, "Complete", 8);
                _dxf_ExGraphDelete (p);
            }
	}
	return;
    }
    curr   = *gq_curr;

    /*
     * Graph is complete, free it
     */
    ExMarkTime(8, "deq module");

    delete = curr->func;
    tofree = (Pointer) curr;

    more = (gq_head != NULL) ? TRUE : FALSE;


    if (delete->runable > 0)
    {
	delete->deleted = TRUE;
	goto unlock;
    }

    DXFree (tofree);

    /* Don't tell the user that his assignements are complete, but do
     * tell the ui that the request was completed.
     */
    if (SIZE_LIST (delete->funcs) > 0)
    {
	DXDebug ("*0", "graph %d completed", delete->graphId);
    }
    /* only the master has to send a complete message to UI */
    if(!_dxd_exRemoteSlave)
        _dxf_ExSPack (PACK_COMPLETE, delete->graphId, "Complete", 8);
    _dxf_ExGraphDelete (delete);

    if (! more)
	ExResetGraphKill ();
unlock:
    *gq_curr = NULL;
    DXunlock (gq_sem, 0);

}


/*
 * Determine if the specified graph is executing.
 */
int
_dxf_ExGQRunning (int graphId)
{
    int		status;

    if (! have_lock)
	DXlock (gq_sem, 0);

    status = *gq_curr != NULL;

    if (! have_lock)
	DXunlock (gq_sem, 0);

    return (status);
}

/*
 * Determine if all graphs have completed execution
 */
int
_dxf_ExGQAllDone (void)
{
    int status;

    /*
     * Avoid the lock if possible
     */

    if (gq_head)
	return (FALSE);

    if (*gq_curr)
	return (FALSE);

    DXlock (gq_sem, 0);

    status = (*gq_curr == NULL	&&
	      (! gq_head));

    DXunlock (gq_sem, 0);

    return (status);
}


void _dxf_ExGQPrint (char *s)
{
    gq_elem	*e;

    printf ("%s: head = %p, tail = %p, Q = ", s, gq_head, gq_tail);
    for (e = gq_head; e; e = e->next)
	printf ("%p ", e);
    printf ("\n");

    printf ("curr = ");
    printf ("%p ", *gq_curr);
    printf ("\n");
}
