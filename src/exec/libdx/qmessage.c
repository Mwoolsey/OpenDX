/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>


#include <stdarg.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <dx/dx.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include "internals.h"

extern Object _dxfExportBin_FP(Object o, int fd);

/*
 * The message queue is maintained as a linked list of messages.
 * The struct queue contains the pointers to the first and last
 * elements of the list, and a DXlock for the list.  It is allocated
 * and initialized by _dxf_initmessages(), which is called by initdx().
 * DXAdd a message to the queue with DXqwrite(); call DXqflush() to flush
 * the queue.  Because the queue elements are allocated, some care
 * is necessary in producing messages from the allocator; thus the
 * _dxfemergency() routine, which causes the queue to be flushed on the
 * next write from this processor.
 */

static struct _dxd_queue {			/* the queue */
    lock_type DXlock;			/* queue DXlock */
    struct element *first;		/* first element of queue */
    struct element *last;		/* last element of queue */
} *queue;				/* the queue */


struct element {			/* an element of the queue */
    struct element *next;		/* next element of the list */
    int fd;				/* file descriptor for this message */
    int length;				/* length of the message itself */
    char message[1];			/* allocate enough to put msg here */
};


static int emergency_flag = 0;		/* whether to flush next DXqwrite */
static int isoutboard = 0;              /* is this an outboard module? */
static int outsocket = -1;              /* open socket to real exec */

void _dxfemergency(void)		/* call to declare an emergency */
{
    emergency_flag = 1;
}


Error
_dxf_initmemqueue()
{
    queue = (struct _dxd_queue *) DXAllocate(sizeof(struct _dxd_queue));
    if (!queue)
	return ERROR;
    DXcreate_lock(&queue->DXlock, "message queue");
    queue->first = NULL;
    queue->last = NULL;
    return OK;
}


void
DXqwrite(int fd, char *message, int length)
{
    struct element *e;
    if (DXProcessorId()!=0 &&
	queue &&
	!emergency_flag &&
	(e = (struct element *) DXAllocate(sizeof(struct element)+length))
    ) {
	/* queue the message */
	e->next = NULL;
	e->fd = fd;
	e->length = length;
	memcpy(e->message, message, length);
	DXlock(&queue->DXlock, 0);
	if (queue->last)
	    queue->last->next = e;
	else
	    queue->first = e;
	queue->last = e;
	DXunlock(&queue->DXlock, 0);
    } else {
	/* flush queued, write current message */
	DXqflush();
	emergency_flag = 0;
	if (write(fd, message, length)<0)
	    perror("DXqwrite");
    }	       
}


void
DXqflush()
{
    struct element *e;
    if (!queue)
	return;
    while (queue->first) {
	DXlock(&queue->DXlock, 0);
	e = queue->first;
	queue->first = e->next;
	if (!queue->first)
	    queue->last = NULL;
	DXunlock(&queue->DXlock, 0);
	if (write(e->fd, e->message, e->length) < 0)
	    perror("DXqwrite");
	DXFree((Pointer)e);
    }
}


#if !defined(pgcc) && !defined(intelnt) && !defined(WIN32) && !defined(macos)
#if DXD_OS2_SYSCALL
void _Optlink
#else
void
#endif
exit(int n)
{
    fclose(stdout);
    fclose(stderr);
    DXqflush();
    _exit(n);
}
#endif


static void 
outboardmsg(char *who, char *message, va_list arg)
{
    char buf[2000];
    Group g;
    String whoObj;
    String msgObj;
    Array oneObj;
    int n;

    g = DXNewGroup();
    if (who)
	whoObj = DXNewString(who);
    else
	whoObj = DXNewString("MESSAGE");

    n = 1;
    oneObj = DXAddArrayData (DXNewArray (TYPE_INT, CATEGORY_REAL, 0), 
			0, 1, (Pointer) &n);

    vsprintf(buf, message, arg);
    msgObj = DXNewString(buf);
    
    if (!g || !whoObj || !msgObj || !oneObj)
	goto clean_up;
    
    if (!DXSetEnumeratedMember(g, 0, (Object)whoObj))
	goto clean_up;
    whoObj = NULL;
    if (!DXSetEnumeratedMember(g, 1, (Object)msgObj))
	goto clean_up;
    msgObj = NULL;
    
    if (!DXSetAttribute((Object)g, "message", (Object)oneObj))
	goto clean_up;
    oneObj = NULL;
    
    if (!_dxfExportBin_FP((Object)g, outsocket))
	goto clean_up;
    
  clean_up:
    DXDelete((Object)g);
    DXDelete((Object)whoObj);
    DXDelete((Object)msgObj);
    DXDelete((Object)oneObj);
}

/*
 * DXsqmessage() is standalone DXqmessage: called by DXqmessage() to
 * produce standalone and executive script mode output.
 */

void
DXsqmessage(char *who, char *message, va_list arg)
{
    char buf[2000];
    int n;

    if (isoutboard) {
	outboardmsg(who, message, arg);
	return;
    }

    if (who && ! strcmp (who, "LINK"))
        return;

    if (!who || strcmp(who, "MESSAGE"))
	sprintf(buf, "%2d: ", DXProcessorId());
    else if (*who=='*')
	buf[0] = '\0';
    else
	sprintf(buf, "%2d: %s: ", DXProcessorId(), who);
    vsprintf(buf+strlen(buf), message, arg);

    n = strlen(buf);
    buf[n] = '\n';
    DXqwrite(1, buf, n+1);
}		

Error _dxfSetOutboardMessage(int OutSockFD)
{
    isoutboard = 1;
    outsocket = OutSockFD;
    return OK;
}

Error _dxfResetOutboardMessage()
{
    isoutboard = 0;
    outsocket = -1;
    return OK;
}



/* cover function to call standalone message code if the library is 
 * not being linked with the executive.
 */
void
DXqmessage(char *who, char *message, va_list args)
{
    DXsqmessage(who, message, args);
}


