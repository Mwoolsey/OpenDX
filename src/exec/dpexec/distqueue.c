/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>


#include <dx/dx.h>
#include "config.h"
#include "distp.h"

/*
 * Initialize the structures for the dpsend queue.
 */
Error _dxf_ExSendQInit (DPSendQ *sendq)
{
    sendq->head = NULL;
    sendq->tail = NULL;

    return (OK);
}

/*
 * Insert an entry into the dpsend queue.
 */
void _dxf_ExSendQEnqueue (DPSendQ *sendq, DPSendPkg *pkg)
{
    dps_elem	*elem;

    if (pkg == NULL)
	return;

    if ((elem = (dps_elem *) DXAllocate (sizeof (dps_elem))) == NULL)
	_dxf_ExDie ("_dxf_ExSendQEnqueue:  can't Allocate");

    elem->pkg   = pkg;
    elem->next  = NULL;

    if (sendq->head)
        (sendq->tail)->next = elem;		/* add to end */
    else
        sendq->head = elem;			/* new list */

    sendq->tail = elem;

}

/*
 * Remove an entry from the dpsend queue.
 */
DPSendPkg *_dxf_ExSendQDequeue (DPSendQ *sendq)
{
    dps_elem	*elem;
    DPSendPkg	*pkg;

    if (sendq->head == NULL)
	return (NULL);

    elem = sendq->head;
    sendq->head = elem->next;

    if (sendq->tail == elem)			/* last one */
	sendq->tail = NULL;

    pkg = elem->pkg;
    DXFree ((Pointer)elem);

    return (pkg);
}

int _dxf_ExSendQEmpty(DPSendQ *sendq)
{
    if(sendq->head)
        return(FALSE);
    else return(TRUE);
}
