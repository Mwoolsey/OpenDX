/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"




#include "XHandler.h"
#include "ListIterator.h"


List XHandler::Handlers;

XHandler::XHandler(int eventName,
		   Window window,
		   XHandlerCallback cb,
		   void *clientData)
{
    XHandler::Handlers.insertElement(this, 1);

    this->eventName  = eventName;
    this->window     = window;
    this->callback   = cb;
    this->clientData = clientData;
}

XHandler::~XHandler()
{
    XHandler::Handlers.removeElement(this);
}

//
// Routine to process events.
//
boolean XHandler::ProcessEvent(XEvent *event)
{
    ListIterator li(XHandler::Handlers);
    XHandler *h;
    while( (h = (XHandler*)li.getNext()) )
    {
	if ((h->window == 0 || h->window == event->xany.window) &&
	    h->eventName == event->xany.type)
	{
	    if (!(*h->callback)(event, h->clientData))
		return FALSE;
	}
    }

    return TRUE;
}
