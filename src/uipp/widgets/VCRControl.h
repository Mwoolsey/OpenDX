/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>


#ifndef _VCRControl_h
#define _VCRControl_h

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif


#include "XmDX.h"

extern  WidgetClass                   xmVCRControlWidgetClass;
typedef struct _XmVCRControlClassRec  *XmVCRControlWidgetClass;
typedef struct _XmVCRControlRec       *XmVCRControlWidget;

typedef struct {
    int     action;
    Boolean on;
    char    which;
    int     detent;
    short     value;
} VCRCallbackStruct;

extern Widget XmCreateVCRControl(Widget parent, String name, 
                                 ArgList args, Cardinal num_args);

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif

#endif
