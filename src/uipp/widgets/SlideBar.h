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
 * $Log: SlideBar.h,v $
 * Revision 1.5  1999/05/10 15:46:33  gda
 * Copyright message
 *
 * Revision 1.5  1999/05/10 15:46:33  gda
 * Copyright message
 *
 * Revision 1.4  1999/05/03 14:09:04  gda
 * dxconfig.h, plus other post-new-shapshot fixes
 *
 * Revision 1.3  1999/04/30 13:10:08  gda
 * After re-snapshotting, ripping out headers and adding linux changes
 *
 * Revision 9.1  1997/05/22 18:48:26  svs
 * Copy of release 3.1.4 code
 *
 * Revision 8.0  1995/10/03  18:50:07  nsc
 * Copy of release 3.1 code
 *
 * Revision 7.1  1994/01/18  19:02:50  svs
 * changes since release 2.0.1
 *
 * Revision 6.1  93/11/16  10:51:08  svs
 * ship level code, release 2.0
 * 
 * Revision 1.1  93/02/26  15:47:33  danz
 * Initial revision
 * 
 * 
 */

#ifndef SlideBar_H
#define SlideBar_H

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

#include "XmDX.h"

extern WidgetClass xmSlideBarWidgetClass;

typedef struct _XmSlideBarClassRec * XmSlideBarWidgetClass;
typedef struct _XmSlideBarRec      * XmSlideBarWidget;

typedef struct {
    unsigned char reason;
    unsigned char indicator;
    short value;
} SlideBarCallbackStruct;


/*  Declare types for convenience routine to create the widget  */
extern Widget XmCreateSlideBar
  (Widget parent, String name, ArgList args, Cardinal num_args);

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif

#endif
