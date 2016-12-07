/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/
/*
 * $Header: /src/master/dx/src/exec/dxmods/bounds.h,v 1.4 2000/08/24 20:04:24 davidt Exp $
 */

#include <dxconfig.h>

#ifndef _BOUNDS_H_
#define _BOUNDS_H_

/* general bounding box functions */

Object _dxf_BBoxPoint(Object o, Point *p, int flag);

#define BB_CENTER               0       /* center of volume */

#define BB_FRONT                1       /* center of front face */
#define BB_BACK                 2       /* center of back face */
#define BB_RIGHT                3       /* center of right face */
#define BB_LEFT                 4       /* center of left face */
#define BB_TOP                  5       /* center of top face */
#define BB_BOTTOM               6       /* center of bottom face */

#define BB_FRONTTOPRIGHT        7       /* corners */
#define BB_FRONTTOPLEFT         8
#define BB_FRONTBOTRIGHT        9
#define BB_FRONTBOTLEFT        10
#define BB_BACKTOPRIGHT        11
#define BB_BACKTOPLEFT         12
#define BB_BACKBOTRIGHT        13 
#define BB_BACKBOTLEFT         14


Object _dxf_BBoxDistance(Object o, float *distance, int flag);

#define BB_WIDTH                0       /* box lengths */
#define BB_HEIGHT               1
#define BB_DEPTH                2
#define BB_DIAGONAL             3

#endif /* _BOUNDS_H_ */
