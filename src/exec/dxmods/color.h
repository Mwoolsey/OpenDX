/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/
/*
 * $Header: /src/master/dx/src/exec/dxmods/color.h,v 1.1 2000/08/24 20:04:26 davidt Exp $
 */

#include <dxconfig.h>

#ifndef  _COLOR_H_
#define  _COLOR_H_

#include <dx/dx.h>

/* from color.c */
int   _dxfIsImportedColorMap(Object, int *, Object *, Object *);
Error _dxfIsColorMap (Object);
Error _dxfIsOpacityMap (Object);
Error _dxfColorRecurseToIndividual (Object, Object,
                                    Object, char *, int, int, int, int, 
				    RGBColor, float, int); 

/* from colorbar.c */
Error _dxfTransposePositions(Array array);
Error _dxfGetColorBarAnnotationColors(Object, Object,
                   RGBColor, RGBColor, RGBColor *, RGBColor *,
                   RGBColor *, int *);


#endif /* _COLOR_H_ */

