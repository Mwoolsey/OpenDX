/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"


#include <string.h>
#if defined(HAVE_MALLOC_H)
#include <malloc.h>
#endif
#include <stdio.h>

#define Object XObject
#include <X11/Intrinsic.h>
#undef Object

#include "dxlP.h"

typedef struct
{
    XtWorkProcId wpid;
    XtInputId    iid;
    XtAppContext appContext;
} XEventHandlerData;

static Boolean
_dxl_WorkProcHandler(XtPointer  clientData)
{
    DXLConnection *conn = (DXLConnection *)clientData;
    XEventHandlerData *xehd = (XEventHandlerData *)conn->eventHandlerData;

    if (xehd->iid && conn->nEvents > 0)
	DXLHandlePendingMessages(conn);

    xehd->wpid = 0;

    return True;
}

static void
_dxl_XInstallWorkProc(DXLConnection *c)
{
    XEventHandlerData *xehd = (XEventHandlerData *)c->eventHandlerData;
    if (xehd && xehd->wpid == 0)
    {
	xehd->wpid = XtAppAddWorkProc(xehd->appContext, 
			   _dxl_WorkProcHandler, (XtPointer)c);
    }
}

static void
_dxl_XRemoveWorkProc(DXLConnection *c)
{
    XEventHandlerData *xehd = (XEventHandlerData *)c->eventHandlerData;
    if (xehd && xehd->wpid != 0)
    {
	XtRemoveWorkProc(xehd->wpid);
	xehd->wpid = 0;
    }
}


static Boolean
_dxl_InputHandler(XtPointer  clientData,
				   int       *socket,
				   XtInputId *id)
{
    DXLConnection *conn = (DXLConnection *)clientData;
    DXLHandlePendingMessages(conn);

    _dxl_XRemoveWorkProc(conn);

    return False;
}

static void
_dxl_uninitializeXMainLoop(DXLConnection *conn)
{
    XEventHandlerData *xehd = (XEventHandlerData *)conn->eventHandlerData;

    if (xehd->iid)
	XtRemoveInput((XtInputId)(xehd->iid));
    
    if (xehd->wpid)
	XtRemoveWorkProc((XtWorkProcId)(xehd->wpid));
    
    xehd->iid = 0;
    xehd->wpid = 0;

    FREE(conn->eventHandlerData);
    conn->eventHandlerData = NULL;
}

DXLError
DXLUninitializeXMainLoop(DXLConnection *conn)
{
    _dxl_uninitializeXMainLoop(conn);
    return OK;
}

DXLError
DXLInitializeXMainLoop(XtAppContext app, DXLConnection *conn)
{
    XEventHandlerData *xehd;

    conn->eventHandlerData = (void *)MALLOC(sizeof(XEventHandlerData));
    if (! conn->eventHandlerData)
	return ERROR;
    
    memset(conn->eventHandlerData, 0, sizeof(XEventHandlerData));
    
    xehd = (XEventHandlerData *)conn->eventHandlerData;

    conn->removeEventHandler = _dxl_uninitializeXMainLoop;
    conn->installIdleEventHandler = _dxl_XInstallWorkProc;
    conn->clearIdleEventHandler = _dxl_XRemoveWorkProc;

    xehd->appContext = app;
    xehd->wpid = 0;
    xehd->iid = XtAppAddInput(app, DXLGetSocket(conn), 
#if defined(intelnt)
			       (XtPointer)XtInputReadWinsock,
#else
			       (XtPointer)XtInputReadMask,
#endif
			       (XtInputCallbackProc)_dxl_InputHandler,
			       (XtPointer)conn);

    return TRUE;
}
	

