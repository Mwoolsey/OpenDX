/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/
/*
 * $Header: /src/master/dx/src/exec/dxmods/_autocolor.h,v 1.1 2000/08/24 20:04:06 davidt Exp $
 */

#include <dxconfig.h>


#ifndef  __AUTOCOLOR_H_
#define  __AUTOCOLOR_H_

#include <dx/dx.h>

Error _dxfByteField(Object, int *);
Group _dxfAutoColor(Object,  float, float, float,
           float, float, float *, float *, Object *, int,
           RGBColor, RGBColor);
Error _dxfAutoColorObject(Object, Object, Object);
Error _dxfHSVtoRGB(float, float, float, float *, float *, float *);
int   _dxfFieldWithInformation(Object);
Error _dxfDelayedOK(Object);
Error _dxfRGBtoHSV(float, float, float, float *, float *, float *);
Error _dxfSetMultipliers(Object, float, float);
Error _dxfScalarField(Field, int *);
Error _dxfBoundingBoxDiagonal(Object, float *);
Field _dxfMakeRGBColorMap(Field);
int   _dxfIsVolume(Object, int *);
Field _dxfMakeHSVfromRGB(Field);
Error _dxfIsTextField(Field, int *);
Error _dxfFloatField(Object, int *);


#endif /* __AUTOCOLOR_H_ */

