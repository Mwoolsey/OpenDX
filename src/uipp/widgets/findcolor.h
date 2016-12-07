/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>


#ifndef _findcolor_h
#define _findcolor_h

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

extern
void find_color(Widget w, XColor *target);

extern
void find_color_ronly(Widget w, XColor *target, char *dontuse);

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif

#endif

