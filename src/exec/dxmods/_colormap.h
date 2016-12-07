/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/
/*
 * $Header: /src/master/dx/src/exec/dxmods/_colormap.h,v 1.1 2000/08/24 20:04:08 davidt Exp $
 */

#include <dxconfig.h>

#ifndef  __COLORMAP_H_
#define  __COLORMAP_H_

#include <dx/dx.h>

Field _dxfcolorfield(float *, float*, int, int );
Field _dxfeditor_to_hsv(Array huemap,Array satmap,Array valmap);
Field _dxfeditor_to_opacity(Array opmap);

#endif /* __COLORMAP_H_ */

