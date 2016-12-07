/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>



#ifndef _gamma_h
#define _gamma_h
#include <X11/Xlib.h>

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

extern
void gamma_correct(XColor *cell_def);

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif


#endif
