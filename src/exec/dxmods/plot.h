/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/
/*
 * $Header: /src/master/dx/src/exec/dxmods/plot.h,v 1.3 2000/08/24 20:04:43 davidt Exp $
 */

#include <dxconfig.h>

#ifndef _PLOT_H_
#define _PLOT_H_

#include <dx/dx.h>

#define ZERO_HEIGHT_FRACTION 0.2
Error   _dxfHowManyStrings(Object, int *);
Object  _dxfAxes2D(Pointer p); 
Pointer _dxfNew2DAxesObject(void);
Error   _dxfFreeAxesHandle(Pointer p);
Error   _dxfSet2DAxesCharacteristic(Pointer p, char *characteristic,
                                  Pointer value);
void _dxfaxes_delta(float *, float *, float, int *,
           float *, float *, char *, int *, int, int);
int _dxfHowManyTics(int, int, float, float, float, float, int,
           int *, int *);


#endif /* _PLOT_H_ */
