/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>



#ifndef Image_H
#define Image_H

#include "XmDX.h"

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif


extern WidgetClass xmImageWidgetClass;

typedef struct _XmImageClassRec * XmImageWidgetClass;
typedef struct _XmImageRec      * XmImageWidget;

typedef struct {
    unsigned char reason;
    unsigned char indicator;
    short value;
} ImageCallbackStruct;


/*  Declare types for convenience routine to create the widget  */
extern Widget XmCreateImage
  (Widget parent, String name, ArgList args, Cardinal num_args);
extern void XmImageSetBackgroundPixmap(Widget iw, Pixmap pixmap);
extern void XmImageSetBackgroundPixel(Widget iw, Pixel pixel);

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif

#endif
