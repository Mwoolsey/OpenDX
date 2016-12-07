/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>



/*
 * Implementation of operations on type Vector
 * XXX - these work for Vector and Point too - should that be separated out?
 */

#include <dx/dx.h>

#define Point3 Point
#define Matrix3 Matrix
#define c1 x
#define c2 y
#define c3 z
#define Neg3 DXNeg
#define Add3 DXAdd
#define Sub3 DXSub
#define Mul3 DXMul
#define Div3 DXDiv
#define Min3 DXMin
#define Max3 DXMax
#define Dot3 DXDot
#define Cross3 DXCross
#define Length3 DXLength
#define Normalize3 DXNormalize
#define Apply3 DXApply
#define Transpose3 DXTranspose
#define Invert3 DXInvert
#define AdjointTranspose3 DXAdjointTranspose
#define Determinant3 DXDeterminant
#include "v3.c"
