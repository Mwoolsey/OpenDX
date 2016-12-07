/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>

#include <dx/dx.h>

typedef Error (*PFE)();

#define Object XObject
#define String XString
#define Screen XScreen
#include <Xm/Xm.h>
#undef Object
#undef String
#undef Screen

extern PFE _dxd_registerIHProc;
static XtAppContext appcontext = NULL;
Error _dxf_RegisterInputHandlerX(PFE func, int fd, Pointer arg);

void DXInitializeXMainLoop(XtAppContext app)
{
    appcontext = app;
    _dxd_registerIHProc = _dxf_RegisterInputHandlerX;
}

typedef struct
{
    XtPointer data;
    Error (*func)(int, Pointer);
} MyEvent;

static Boolean
_rihProc(XtPointer client, int *socket, XtInputId *id)
{
    MyEvent *mev = (MyEvent *)client;
    return (*mev->func)(*socket, mev->data);
}

Error
_dxf_RegisterInputHandlerX(Error (*func)(int, Pointer), int fd, Pointer arg)
{
    MyEvent *mev  = (MyEvent *)DXAllocate(sizeof(MyEvent));

    mev->func = func;
    mev->data = (XtPointer)arg;

    XtAppAddInput(appcontext, fd, (XtPointer)XtInputReadMask,
        (XtInputCallbackProc)_rihProc, (XtPointer)mev);

    return OK;
}
