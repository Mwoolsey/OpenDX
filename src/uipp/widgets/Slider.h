/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>


/*	Construct and manage the "frame sequencer guide" to be used as a
 *	popup of the "sequence controller" (alias VCR control)
 *
 *	August 1990
 *	IBM T.J. Watson Research Center
 *	R. T. Maganti
 */

/*
 */

#ifndef Slider_H
#define Slider_H

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

#include "XmDX.h"

/*  Declare types for convenience routine to create the widget  */
extern Widget XmCreateSlider
  (Widget parent, String name, ArgList args, Cardinal num_args);

/*
 * Add a warning callback to the number widget within the slider
 */
extern void
XmSliderAddWarningCallback(Widget w, XtCallbackProc proc, XtPointer clientData);

extern WidgetClass xmSliderWidgetClass;

typedef struct _XmSliderClassRec * XmSliderWidgetClass;
typedef struct _XmSliderRec      * XmSliderWidget;


typedef struct {
    unsigned char reason;
    double value;
} XmSliderCallbackStruct;

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif

#endif
