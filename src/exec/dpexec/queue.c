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
#include "queue.h"


#define	Q_LOCK(l,q)		if (l) DXlock (&(q)->lock, exJID)
#define	Q_UNLOCK(l,q)		if (l) DXunlock (&(q)->lock, exJID)


typedef struct _EXQueueElement
{
    EXQueueElement		next;	/* the element chain		*/
    EXQueueElement		prev;
    Pointer			val;	/* the element's data		*/
} _EXQueueElement;


typedef struct _EXQueue
{
    PFI			enqueue;	/* called when adding element	*/
    PFI			dequeue;	/* called when removing element	*/
    PFI			print;		/* called to print   an element	*/
    PFI			destroy;	/* called to destroy an element	*/
    int			items;		/* number in queue		*/
    int			locking;	/* is locking required		*/
    int			local;		/* is this a local queue	*/
    EXQueueElement	free;		/* element free list		*/
    lock_type		lock;		/* lock if necessary		*/
    EXQueueElement	head;		/* head of the list		*/
    EXQueueElement	tail;		/* tail of the list		*/
} _EXQueue;


static int	ExQueueDefaultPrint	(char *buf, Pointer val);
static int	ExQueueDefaultDestroy	(Pointer val);
static Error	ExQueueInsert		(EXQueue q, Pointer val, int head);
static Pointer	ExQueueRemove		(EXQueue q, int head);


/*
 * Constructs a new queue based upon the following characteristics:
 *
 * local	Whether queue should reside in local memory
 * locking	Whether queue requires locking
 * enqueue	Element enqueuing  function
 * dequeue	Element dequeuing  function
 * print	Element printing   function
 * destroy	Element destructor function
 *
 * If the printing function is defaulted then an appropriate one is supplied.
 */

EXQueue _dxf_ExQueueCreate (int local, int locking,
		       PFI enqueue, PFI dequeue, PFI print, PFI destroy)
{
    int			n;
    EXQueue		q;
    Error		l;

    /*
     * Local queues don't need locking.
     */

    locking = local ? FALSE : locking;

    /*
     * Supply default functions if necessary.
     */

    print   = print   ? print   : ExQueueDefaultPrint;
    destroy = destroy ? destroy : ExQueueDefaultDestroy;

    /*
     * OK, now let's construct the queue and fill in the blanks.
     */

    n = sizeof (_EXQueue);
    q = (EXQueue) (local ? DXAllocateLocal (n) : DXAllocate (n));
    if (! q)
	_dxf_ExDie ("_dxf_ExQueueCreate:  allocate failed");
    ExZero (q, n);

    q->enqueue	= enqueue;
    q->dequeue	= dequeue;
    q->print	= print;
    q->destroy	= destroy;

    q->locking	= locking;
    q->local	= local;

    if (locking)
    {
	l = DXcreate_lock (&q->lock, "Queue");
	if (l != OK)
	    _dxf_ExDie ("_dxf_ExQueueCreate:  DXcreate_lock failed");
    }

    return (q);
}


/*
 * Destroys a queue.  We need not bother with locking here since we
 * assume that a queue will only be destroyed when no-one else is 
 * going to use it.
 */

Error _dxf_ExQueueDestroy (EXQueue q)
{
    EXQueueElement	e, ne;
    PFI			rm;

    if (! q)
	return (OK);
    
    /*
     * Get rid of the free element list.
     */

    for (e = q->free; e; e = ne)
    {
	ne = e->next;
	DXFree ((Pointer) e);
    }

    /*
     * Destroy all of the elements in the queue.
     */

    rm = q->destroy;
    
    for (e = q->head; e; e = ne)
    {
	ne = e->next;
	(* rm) (e->val);
	DXFree ((Pointer) e);
    }

    if (q->locking)
	DXdestroy_lock (&q->lock);
    
    DXFree ((Pointer) q);

    return (OK);
}


/*
 * Calls down to the real removal routine which specify the proper
 * end from which to remove the element.
 */

Pointer _dxf_ExQueueDequeue (EXQueue q)
{
    return (ExQueueRemove (q, TRUE));
}


Pointer _dxf_ExQueuePop (EXQueue q)
{
    return (ExQueueRemove (q, TRUE));
}


/*
 * Calls down to the real insertion routine which specify the proper
 * end at which to place the element.
 */

Error _dxf_ExQueueEnqueue (EXQueue q, Pointer val)
{
    return (ExQueueInsert (q, val, FALSE));
}


Error _dxf_ExQueuePush (EXQueue q, Pointer val)
{
    return (ExQueueInsert (q, val, TRUE));
}


int _dxf_ExQueueElementCount (EXQueue q)
{
    if (! q)
	return (0);
    return (q->items);
}


void _dxf_ExQueuePrint (EXQueue q)
{
    int			l;
    EXQueueElement	e;
    int			i;
    char		buf[4096];

    l  = q->locking;
    /*pr = q->print;*/

    Q_LOCK (l, q);

    DXMessage ("#1220", q, q->items, q->local   ? 'T' : 'F', q->locking ? 'T' : 'F');

    for (i = 0, e = q->head; e; i++, e = e->next)
    {
	sprintf (buf, "[%4d] %p <%p %p> ", i, e, e->prev, e->next);
	(* q->print) (buf + strlen (buf), e->val);
	DXMessage (buf);
    }

    Q_UNLOCK (l, q);
}

/****************************************************************************/

/*
 * Default queue element destruction routine.
 */

static int ExQueueDefaultDestroy (Pointer val)
{
    return (0);
}


/*
 * Default queue element printing routine.
 */

static int ExQueueDefaultPrint (char *buf, Pointer val)
{
    return (sprintf (buf, "0x%08lx", (long)val));
}


/*
 * The real queue insertion routine.  If the head flag is true then this
 * is a stack push and the element is inserted in the front of the queue,
 * otherwise this is a normal enqueuing and the element has to go to the 
 * back of the queue.
 */

static Error ExQueueInsert (EXQueue q, Pointer val, int head)
{
    PFI			enq;
    int			l;
    EXQueueElement	e;
    int			s;

    DXsyncmem ();

    enq = q->enqueue;
    l   = q->locking;

    /*
     * Perform the enqueueing function if one exists.
     */

    if (enq)
	(* enq) (val);

    /*
     * Pick up a free element block.  If the free list doesn't have any then
     * let go of the lock for a bit while we allocate one.
     */

    Q_LOCK (l, q);

    if ((e = q->free) == NULL)
    {
	Q_UNLOCK (l, q);
	s = sizeof (_EXQueueElement);
	e = (EXQueueElement) (q->local ? DXAllocateLocal (s) : DXAllocate (s));
	if (! e)
	    _dxf_ExDie ("ExQueueInsert:  can't allocate");
	Q_LOCK (l, q);
    }
    else
	q->free = e->next;

    e->val = val;

    /*
     * DXInsert the element at the requested end of the list.
     */

    if (q->items)
    {
	if (head)
	{
	    e->next = q->head;
	    e->prev = NULL;
	    e->next->prev = e;
	    q->head = e;
	}
	else
	{
	    e->next = NULL;
	    e->prev = q->tail;
	    e->prev->next = e;
	    q->tail = e;
	}
    }
    else
    {
	e->next = e->prev = NULL;
	q->head = q->tail = e;
    }

    q->items++;

    Q_UNLOCK (l, q);

    return (OK);
}


/*
 * The real queue removal routine.  If the head flag is true then this
 * the element is taken from the front of the queue, otherwise it is 
 * taken from the end.
 */

static Pointer ExQueueRemove (EXQueue q, int head)
{
    PFI			deq;
    int			l;
    EXQueueElement	e;
    int			n;
    Pointer		val;

    DXsyncmem ();

    if (q->items == 0)
	return (NULL);

    deq = q->dequeue;
    l   = q->locking;

    Q_LOCK (l, q);

    /*
     * DXRemove the element from the list and decrement the count of elements
     * in the queue.
     */

    n = --(q->items);

    if (head)
    {
	e = q->head;
	q->head = e->next;
	if (n)
	    e->next->prev = NULL;
	else
	    q->tail = NULL;
    }
    else
    {
	e = q->tail;
	q->tail = e->prev;
	if (n)
	    e->prev->next = NULL;
	else
	    q->head = NULL;
    }

    /*
     * Return the element block to the queue's free list.
     */

    val = e->val;
    e->next = q->free;
    q->free = e;

    Q_UNLOCK (l, q);

    /*
     * Call the dequeuing function if one exists.
     */

    if (deq)
	(* deq) (val);

    return (val);
}


#if 0

ExWorker_TestQueue ()
{
    extern int		exDEBUGQ; /* defd out */
    EXQueue		q;
    int			i;
    int			n;

#if 1
    q = _dxf_ExQueueCreate (FALSE, TRUE, NULL, NULL, NULL, NULL);
#else
    q = (EXQueue) exDEBUGQ;
#endif

    for (i = 0; i < 32; i++)
    {
	n = i << 8;
	n &= 0xffffff00;
	n |= exJID;
	i & 1 ? _dxf_ExQueueEnqueue (q, (Pointer) n)
	      : _dxf_ExQueuePush    (q, (Pointer) n);
    }

    for (i = 0; i < 32; i++)
	DXMessage ("#1230", _dxf_ExQueuePop (q));

    if (exJID == 1)
    {
	_dxf_ExQueuePrint (q);
    }

    pause ();
}

#endif
