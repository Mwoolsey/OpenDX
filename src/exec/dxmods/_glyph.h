/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/
/*
 * $Header: /src/master/dx/src/exec/dxmods/_glyph.h,v 1.1 2000/08/24 20:04:12 davidt Exp $
 */

#include <dxconfig.h>


#ifndef  __GLYPH_H_
#define  __GLYPH_H_

#include <dx/dx.h>

Error _dxfGetTensorStats(Object, float *, float *);
int   _dxfIsTensor(Object);
Error _dxfProcessGlyphObject(Object, Object, char *,
                             float, float, int, float,
                             int, float, int,
                             float, int, float, int,
                             Object, int, int, char *, int *, float *);
Error _dxfIndexGlyphObject(Object, Object, float);
Error _dxfGetFontName(char *, char *);
Error _dxfGlyphTask(Pointer);
Error _dxfGetStatisticsAndSize(Object, char *, int, float *, int,
                               float *, float *, int *);
Error _dxfTextField(Pointer);
int   _dxfIsFieldorGroup(Object);
Error _dxfWidthHeuristic(Object, float *);





#endif /* __GLYPH_H_ */

