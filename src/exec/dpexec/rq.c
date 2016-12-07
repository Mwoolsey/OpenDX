/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/
#include <dx/dx.h>
#include <dxconfig.h>

#include "config.h"
#include "rq.h"
#include "instrument.h"
#include "swap.h"
#include "dxmain.h"
#include "parse.h"

#define MARK_TIME(s) /* DXMarkTimeLocal(s) */

typedef struct _EXRQJob         *EXRQJob; 

typedef struct _EXRQJob
{
    EXRQJob	next;			/* list pointers		*/
    EXRQJob	prev;
    int		highpri;		/* run this NOW			*/
    long	gid;			/* group id			*/
    int		JID;			/* job   id			*/
    PFI		func;			/* function to call		*/
    Pointer	arg;			/* argument to pass		*/
    int		repeat;			/* number of repetitions for job*/
} _EXRQJob;


typedef struct
{
    volatile int        count;
    volatile EXRQJob    free;
    volatile EXRQJob    head;
    volatile EXRQJob    tail;
} _EXRQ, *EXRQ;

static EXRQ runQueue = NULL;
static lock_type *RQlock;
static int send_RQ_message = 1;

Error _dxf_ExRQInit (void)
{
    Error tmp;
    if (runQueue != NULL)
	return (ERROR);

    runQueue = (EXRQ) DXAllocate (sizeof (_EXRQ)); 
    if (runQueue == NULL)
	return (ERROR);

    RQlock = (lock_type*) DXAllocate (sizeof (lock_type));
    if (!RQlock) {
	DXFree((Pointer)runQueue);
	return ERROR;
    }

    tmp = DXcreate_lock ((lock_type *) RQlock, "Runqueue");
    if (tmp != OK) {
	DXFree((Pointer)runQueue);
	DXFree((Pointer)RQlock);
	return (ERROR);
    }

    runQueue->count = 0;
    runQueue->free = NULL;
    runQueue->head = NULL;
    runQueue->tail = NULL;

    return (OK);
}

void _dxf_ExRQEnqueue (PFI func, Pointer arg, int repeat,
		       long gid, int JID, int highpri)
{
    volatile EXRQ	rq;
    lock_type		*l;
    EXRQJob		job	= NULL;
    _EXRQJob		localJob;
    EXRQJob		tail;
    EXRQJob		head;

MARK_TIME ("RQE Enter");
    DXsyncmem();

    localJob.next    = NULL;
    localJob.prev    = NULL;
    localJob.highpri = highpri;
    localJob.gid     = gid;
    localJob.JID     = JID;
    localJob.func    = func;
    localJob.arg     = arg;
    localJob.repeat  = repeat;


    rq = runQueue;
    l  = RQlock;

    /*
     * Prepare a job block.  If we can then get one from the runqueue's
     * own free list.  If someone else sneaks in and gets the last one
     * or there aren't any then we'll just allocate a new one.  Once
     * we've gotten the block then fill it in.
     */

MARK_TIME ("RQE Alloc");
    if (rq->free)
    {
	DXlock (l, exJID);
	job = rq->free;
	if (job)
	    rq->free = job->next;
	else
	{
	    DXunlock (l, exJID);
	    job = (EXRQJob) DXAllocate (sizeof (_EXRQJob));
	    DXlock (l, exJID);
	}
    }
    else 
    {
	job = (EXRQJob) DXAllocate (sizeof (_EXRQJob));
	DXlock (l, exJID);
    }
    
    if (! job)
	_dxf_ExDie ("_dxf_ExRQEnqueue: can't DXAllocate job");
MARK_TIME ("RQE DoneAlloc");
    
    *job = localJob;

    /*
     * Now that the job block is set up insert it into the general list.
     * If the list is currently empty then this job becomes both the head
     * and the tail of the list and we can quit.
     *
     * NOTE: 	high priority jobs are placed at the head of the list, all
     *		others at the tail.
     */
    if (highpri)
    {
	head = rq->head;
	if (! head)
	    rq->head = rq->tail = job;
	else
	{
	    job->next  = head;
	    head->prev = job;
	    rq->head   = job;
	}
    }
    else
    {
	tail = rq->tail;
	if (! tail)
	    rq->head = rq->tail = job;
	else 
	{
	    job->prev  = tail;
	    tail->next = job;
	    rq->tail   = job;
	}
    }

MARK_TIME ("RQE Incr");
    rq->count += repeat;
MARK_TIME ("RQE Queued");
    DXunlock (l, exJID);
MARK_TIME ("RQE Exit");
    if(!_dxf_ExReclaimingMemory()) {
        /* child 1 adding something to child 0's queue */
        if(_dxd_exMyPID == 1 && JID == 1)
            _dxf_parent_RQ_message(); 
        else {
            if(send_RQ_message) {
                send_RQ_message = 0;
                _dxf_ExRunOn (1, _dxf_child_RQ_message, &JID, sizeof(int));
                send_RQ_message = 1;
            }
        }
    }
}

/*
 * Enqueue several jobs.  All allocations and queue building happens up front
 * into a queuePart.
 * the queue is then locked, and the jobs are enqueued all at once.
 */

void _dxf_ExRQEnqueueMany (int n, PFI func[], Pointer arg[], 
			int repeat[], long gid, int JID, int highpri)
{
    volatile EXRQ	rq;
    lock_type		*l;
    _EXRQJob		localJob;
    EXRQJob		tail;
    EXRQJob		head;
    int			i;
    EXRQJob		queuePartHead;
    EXRQJob		queuePartTail;
    EXRQJob		newBlock;
    EXRQJob		prevBlock;
    int			totalRepeat;

MARK_TIME ("RQEM Enter");

    if (n == 0)
	return;

    DXsyncmem();

    rq = runQueue;
    l  = RQlock;

    queuePartHead = NULL;
    queuePartTail = NULL;

MARK_TIME ("RQEM Alloc");
    /* DXAllocate n blocks in a list */
    i = 0;
    if (rq->free) 
    {
	DXlock (l, exJID);
	for (; i < n && rq->free; ++i)
	{
	    newBlock = rq->free;
	    if (i == 0)
		queuePartTail = newBlock;
	    rq->free = newBlock->next;
	    newBlock->next = queuePartHead;
	    queuePartHead = newBlock;
	}
	DXunlock (l, exJID);
    }
    for (; i < n; ++i)
    {
	newBlock = (EXRQJob) DXAllocate (sizeof (_EXRQJob));
	if (!newBlock)
	    _dxf_ExDie ("_dxf_ExRQEnqueueMany: can't allocate newBlock");
	if (i == 0)
	    queuePartTail = newBlock;
	newBlock->next = queuePartHead;
	queuePartHead = newBlock;
    }
MARK_TIME ("RQE DoneAlloc");


    newBlock = queuePartHead;
    prevBlock = NULL;
    totalRepeat = 0;
    localJob.highpri = highpri;
    localJob.gid     = gid;
    localJob.JID     = JID;
    for (i = 0; i < n; ++i) 
    {
	localJob.next    = newBlock->next;
	localJob.prev    = prevBlock;
	localJob.func    = func[i];
	localJob.arg     = arg[i];
	localJob.repeat  = repeat[i];

	totalRepeat += localJob.repeat;
	*newBlock = localJob;

	prevBlock = newBlock;
	newBlock = newBlock->next;
    }

MARK_TIME ("RQEM DoneFill");

    /*
     * Now that the job block is set up insert it into the general list.
     * If the list is currently empty then this job becomes both the head
     * and the tail of the list and we can quit.
     *
     * NOTE: 	high priority jobs are placed at the head of the list, all
     *		others at the tail.
     */

    DXlock (l, exJID);

    if (highpri)
    {
	head = rq->head;
	if (! head)
	{
	    rq->head = queuePartHead;
	    rq->tail = queuePartTail;
	}
	else
	{
	    queuePartTail->next  = head;
	    head->prev = queuePartTail;
	    rq->head   = queuePartHead;
	}
    }
    else
    {
	tail = rq->tail;
	if (! tail)
	{
	    rq->head = queuePartHead;
	    rq->tail = queuePartTail;
	}
	else 
	{
	    queuePartHead->prev  = tail;
	    tail->next = queuePartHead;
	    rq->tail   = queuePartTail;
	}
    }

MARK_TIME ("RQEM Incr");
    rq->count += totalRepeat;
MARK_TIME ("RQEM Queued");
    DXunlock (l, exJID);
MARK_TIME ("RQEM Exit");
    send_RQ_message = 0;
    _dxf_ExRunOn (1, _dxf_child_RQ_message, &JID, sizeof(int));
    send_RQ_message = 1;
}



int _dxf_ExRQDequeue	(long gid)
{
    volatile EXRQ	rq;
    _EXRQ		localRq;
    lock_type		*l;
    EXRQJob		job;
    _EXRQJob		localJob;

MARK_TIME ("RQD Enter");
    rq = runQueue;

    l   = RQlock;

    /*
     * If we got a worker specific job then go ahead and do it.
     */

    DXsyncmem ();

    if (rq->count <= 0)
	return (FALSE);

    DXlock (l, exJID);
MARK_TIME ("RQD PostLock");
    localRq = *rq;

    if (localRq.count <= 0) 
    {
	DXunlock (l, exJID);
MARK_TIME ("RQD FastDone");
	return (FALSE);
    }
    rq->count = --localRq.count;

    /*
     * Get a job, removing it from the queue.
     * There are 3 special cases:
     * 1)  the DeQueuer wants a specific gid (group ID),
     * 2)  the job is destined for a specific JID (Processor ID).
     * 3)  a high priority task exists for a specific JID
     * If either of these cases is required, skip down the queue until
     * you find a job.
     */

MARK_TIME ("RQD GetJob");
    job = localRq.head;
    if (!job)
    {
	rq->count++;
	DXunlock (l, exJID);
	DXSetError (ERROR_INTERNAL, "#8320");
	return (FALSE);
    }

    localJob = *job;

MARK_TIME ("RQD GotJob");
    if (gid != 0 || localJob.JID != 0)
    {
MARK_TIME ("RQD Find MYJob");
	for ( ; job; job = localJob.next)
	{
	    localJob = *job;
	    if ((!gid || localJob.gid == gid || localJob.highpri) && 
		(!localJob.JID || localJob.JID == exJID))
		break;
	}
	if (!job) 
	{
	    rq->count++;
	    DXunlock (l, exJID);
	    return (FALSE);
	}
    }
MARK_TIME ("RQD GotMYJob");

    /*
     * If the job we got is a repeatable one, then just write the local copy
     * back out to the global copy, otherwize, remove it from the queue and add
     * it to the free list.
     */
    if (--localJob.repeat == 0)
    {
	if (localJob.prev)
	    localJob.prev->next = localJob.next;
	else
	    localRq.head = localJob.next;
	
	if (localJob.next)
	    localJob.next->prev = localJob.prev;
	else
	    localRq.tail = localJob.prev;

	job->next = localRq.free;
	localRq.free  = job;
    }
    else
    {
	*job = localJob;
    }
    *rq = localRq; 

MARK_TIME ("RQD DXFree Job");
    DXunlock (l, exJID);

    /*
     * We have a valid job to execute.  Pick up its function and argument,
     * return the job block to the free list, and really execute the 
     * function.
     */

    IFINSTRUMENT (++exInstrument[_dxd_exMyPID].tasks);

    (* localJob.func) (localJob.arg, localJob.repeat);

MARK_TIME ("RQD Done");
    return (TRUE);
}


int _dxf_ExRQPending(void)
{
    return (runQueue->count > 0);
}
