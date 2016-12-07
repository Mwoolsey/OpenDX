/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"



#ifdef OS2
#include <stdlib.h>
#include <types.h>
#endif
#include <X11/Xlib.h>
#include <X11/StringDefs.h>
#include <X11/Intrinsic.h>
#include <Xm/Xm.h>

#include "WorkspaceCallback.h"


void AddConstraintCallback( Widget widget, register WsCallbackList *callbacks,
			    XtCallbackProc callback, void * closure )
{
    register WsCallbackRec *new;

    new = XtNew(WsCallbackRec);
    new->next = NULL;
    new->widget = widget;
    new->closure = closure;
    new->callback = callback; 

    /*  Put it on the end of the list  */
    if (*callbacks) {
	WsCallbackRec *next = *callbacks;
	while (next->next)
	    next = next->next;
	next->next = new;
    } else {
	*callbacks = new;
    }
}
  

/*  Subroutine:	RemoveConstraintCallbacks
 *  Purpose:	Clear out child constraint callback list
 *		(Routine is just like Xt's _XtRemoveAllCallbacks)
 */
void RemoveConstraintCallbacks( WsCallbackList *callbacks )
{
    register WsCallbackRec *cl, *next;
    
    cl = *callbacks;
    while( cl != NULL )
    {
	next = cl->next;
	XtFree((char *)cl);
	cl = next;
    }
    (*callbacks) = NULL;
}


/*  Subroutine:	CallConstraintCallbacks
 *  Purpose:	Call callbacks in a child's constraint callback list
 */
void CallConstraintCallbacks( Widget widget, WsCallbackList callbacks,
			      XmAnyCallbackStruct *call_data )
{
    /*  Put it on the end of the list  */
    while( callbacks != NULL )
    {
	(*callbacks->callback)(widget, callbacks->closure, call_data);
	callbacks = callbacks->next;
    }
}
