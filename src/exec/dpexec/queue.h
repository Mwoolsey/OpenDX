/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/
/*
 * $Header: /src/master/dx/src/exec/dpexec/queue.h,v 1.4 2000/08/11 15:28:13 davidt Exp $
 */

#include <dxconfig.h>

#ifndef	__EX_QUEUE_H
#define	__EX_QUEUE_H

#include <dx/dx.h>
#include "utils.h"

typedef struct	_EXQueue		*EXQueue;
typedef struct	_EXQueueElement		*EXQueueElement;

EXQueue		_dxf_ExQueueCreate		(int local, int locking,
					 PFI enqueue, PFI dequeue,
					 PFI print,   PFI destroy);
Error		_dxf_ExQueueDestroy		(EXQueue q);

Pointer		_dxf_ExQueueDequeue		(EXQueue q);
Error		_dxf_ExQueueEnqueue		(EXQueue q, Pointer val);
Pointer		_dxf_ExQueuePop		(EXQueue q);
Error		_dxf_ExQueuePush		(EXQueue q, Pointer val);

int		_dxf_ExQueueElementCount	(EXQueue q);
void		_dxf_ExQueuePrint		(EXQueue q);


#endif	/* __EX_QUEUE_H */


