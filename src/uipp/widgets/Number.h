/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>



#ifndef _Number_h
#define _Number_h

#include "XmDX.h"
#include "FFloat.h"	/* Define XmRDouble, XmRFloat, XmDoubleCallbackStruct */
#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

#ifndef XmIsNumberWidget
#define XmIsNumberWidget(w) XtIsSubclass(w, xmNumberWidgetClass)
#endif /* XmIsNumberWidget */

/*  Declare types for convenience routine to create the widget  */
extern Widget XmCreateNumber
  (Widget parent, String name, ArgList args, Cardinal num_args);

extern WidgetClass xmNumberWidgetClass;

typedef struct _XmNumberWidgetClassRec * XmNumberWidgetClass;
typedef struct _XmNumberWidgetRec      * XmNumberWidget;

extern double XmGetNumberValue(XmNumberWidget w);
extern void XmShowNumberValue (XmNumberWidget rw);
extern void XmChangeNumberValue (XmNumberWidget rw, double value);
extern void XmHideNumberValue (XmNumberWidget rw);

/*
 * Set the limits on a number widget of any type (INTEGER, FLOAT, DOUBLE).
 */
extern void XmUnlimitNumberBounds(XmNumberWidget w);
extern void XmSetNumberBounds(XmNumberWidget w, double  dmin, double  dmax);
extern void XmGetNumberBounds(XmNumberWidget w, double *dmin, double *dmax);



typedef struct {
    XEvent      *event;
    int         reason;
    char        *message;
} XmNumberWarningCallbackStruct;

#if defined(__cplusplus) || defined(c_plusplus)
 }
#endif

#endif
