/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>



#ifndef STEPPER_H
#define STEPPER_H

#include "XmDX.h"
#include "Number.h"

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

extern WidgetClass xmStepperWidgetClass;
typedef struct _XmStepperClassRec* XmStepperWidgetClass;
typedef struct _XmStepperRec* XmStepperWidget;

/*  Declare types for convenience routine to create the widget  */
extern Widget XmCreateStepper
  (Widget parent, String name, ArgList args, Cardinal num_args);

#if defined(__cplusplus) || defined(c_plusplus)
 }
#endif

#endif
