/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>


#ifndef tdmGather_h
#define tdmGather_h

/*---------------------------------------------------------------------------*\
 $Source: /src/master/dx/src/exec/hwrender/hwGather.h,v $
  Author: Tim Murphy

  This file contains definitions for the 'gather' structure. This structure
  represents a DX  object which has been converted to a form which is more
  pallatable to the render(s).

\*---------------------------------------------------------------------------*/

/*
 * sortT->flags is really:
 *	surface of volume flag:		1bit
 *	primitive type			3bits
 *	side of a volume primitive	4bits
 */
#define SORT_POINT	0x10	/* index into point connections component */
#define SORT_LINE	0x20	/* index into line connections component */
#define SORT_TRI	0x30	/* index into triangle connections component */
#define SORT_QUAD	0x40	/* index into quads connections component */
#define SIDE_OF_TYPE	0x0F	/* get the side of the tetra or cube that
				   this type constant represents */
#define TYPE_FLAG	0x70	/* get type from type constants */
#define SURFACE_FLAG	0x80	/* one bit 'surface of volume' flag */

Error   _dxf_getHwGatherFlags(gatherO go, int *flags);
Error   _dxf_setHwGatherFlags(gatherO go, int flags);
Error   _dxf_getHwGatherLights(gatherO go, listO *list);
Error   _dxf_setHwGatherLights(gatherO go, listO list);
Error   _dxf_getHwGatherAmbientColor(gatherO go, RGBColor *color);
Error   _dxf_setHwGatherAmbientColor(gatherO go, RGBColor color);
Error   _dxf_getHwGatherMaterial(gatherO go, materialO *mat);
Error   _dxf_setHwGatherMaterial(gatherO go, materialO mat);
Error   _dxf_getHwGatherObject(gatherO go, dxObject *object);
Error   _dxf_setHwGatherObject(gatherO go, dxObject object);
Error   _dxf_getHwGatherClipObject(gatherO go, dxObject *clipObject);
Error   _dxf_setHwGatherClipObject(gatherO go, dxObject clipObject);
Error	_dxf_pushHwGatherClipPlanes(gatherO go, int n, Point *p, Vector *v);
Error	_dxf_getHwGatherClipPlanes(gatherO go, int *n, Point **p, Vector **v);
Error	_dxf_popHwGatherClipPlanes(gatherO go);

gatherO _dxf_newHwGather(void);

/* for old-time's sake... */
dxObject     _dxf_gatheredObject(gatherO gather);
dxObject     _dxf_gatheredClipObject(gatherO gather);
unsigned int _dxf_gatheredFlags(gatherO gather);

#endif /* tdmGather_h */
