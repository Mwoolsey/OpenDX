/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/
/*
 * $Header: /src/master/dx/src/exec/dxmods/_autogray.h,v 1.1 2000/08/24 20:04:07 davidt Exp $
 */

#include <dxconfig.h>

#ifndef  __AUTOGRAY_H_
#define  __AUTOGRAY_H_

#include <dx/dx.h>

Group _dxfAutoGrayScale(Object,  float, float, float, float,
                        float, float *, float *, Object *,
                        int, RGBColor, RGBColor);


#endif /* __AUTOGRAY_H_ */

