/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>



/******************************************************
 * Dial.h: Public header file for Dial Widget Class.
 ******************************************************/
#ifndef	DIAL_H
#define DIAL_H

#include "XmDX.h"

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

extern	WidgetClass	xmDialWidgetClass;
extern	Widget		XmCreateDial(Widget, String, ArgList, Cardinal);

typedef	struct	_XmDialClassRec	*XmDialWidgetClass;
typedef struct  _XmDialRec	*XmDialWidget;

typedef	struct	
	{
	int	reason;
	XEvent	*event;
	double	position;
	}	XmDialCallbackStruct;

typedef int    ClockDirection;

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif
#endif
