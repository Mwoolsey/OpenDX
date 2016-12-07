/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>

#include <dx/dx.h>

/*
 * Header: $
 * 
 */

#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#define Screen XScreen
#include <X11/Xlib.h>
#undef Screen


extern int _dxf_SetExceedSocket(int, void *);

Error
DXRegisterWindowHandlerWithCheckProc(Error (*proc) (int, Pointer),
		int (*check)(int, Pointer), Display *d, Pointer arg)
{
    int fd = ConnectionNumber(d);

    if (! DXRegisterInputHandlerWithCheckProc(proc, check, fd, arg))
	return ERROR;

#ifdef EXCEED_SOCKET

    if (! _dxf_SetExceedSocket(fd, (void *)d))
	return ERROR;

#endif
    
    return OK;
    
}

extern int _dxf_SetExceedSocket(int, void *);

Error
DXRegisterWindowHandler(Error (*proc) (int, Pointer), Display *d, Pointer arg)
{
    int fd = ConnectionNumber(d);

    if (! DXRegisterInputHandler(proc, fd, arg))
	return ERROR;

#ifdef EXCEED_SOCKET

    if (! _dxf_SetExceedSocket(fd, (void *)d))
	    return ERROR;
    
#endif
    
    return OK;
    
}

int _dxf_AnyPendingWindowEvent(Display *d)
{
    return XPending(d);
}
