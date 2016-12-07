/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>


#ifndef tdmMatrix_h
#define tdmMatrix_h
/*---------------------------------------------------------------------------*\
 $Source: /src/master/dx/src/exec/hwrender/hwMatrix.h,v $

  Stuff for matrix routines.

\*---------------------------------------------------------------------------*/


void *
  _dxfCreateStack() ;
void
  _dxfInitStack (void *stack, int resolution, float view[4][4], int projection,
		float width, float aspect, float Near, float Far) ;
void
  _dxfFreeStack (void *stack) ;
void
  _dxfSetFuzzValue (void *stack, float fuzz) ;
void
  _dxfSetScreenFuzzFactor (void *stack, float ff) ;
void
  _dxfPushMatrix (void *stack) ;
void
  _dxfPopMatrix (void *stack) ;
void
  _dxfPushViewMatrix (void *stack) ;
void
  _dxfPopViewMatrix (void *stack) ;
void
  _dxfPushViewport (void *stack) ;
void
  _dxfPopViewport (void *stack) ;
void
  _dxfMultMatrix (void *stack, float m[4][4]) ;
void
  _dxfLoadMatrix (void *stack, float m[4][4]) ;
void
  _dxfLoadViewMatrix (void *stack, float m[4][4]) ;
void
  _dxfSetViewport (void *stack, int left, int right, int bottom, int top) ;
void
  _dxfGetMatrix (void *stack, float m[4][4]) ;
void
  _dxfGetViewMatrix (void *stack, float m[4][4]) ;
void
  _dxfGetViewport (void *stack, int *left, int *right, int *bottom, int *top) ;
void
  _dxfGetCompositeMatrix (void *stack, float m[4][4]) ;
void
  _dxfGetProjectionInfo (void *stack, int *projection,
			float *width, float *aspect, float *Near, float *Far) ;
void
  _dxfSetProjectionInfo (void *stack, int projection,
			float width, float aspect, float Near, float Far) ;
void
  _dxfGetProjectionMatrix (void *context, void *stack, float projmat[4][4]) ;

void
  _dxfMult44 (register double s0[4][4], register double s1[4][4]) ;
void
  _dxfMult44f (register double s0[4][4], register float s1[4][4]) ;
void
  _dxfInverse (register double s0[4][4], register double s1[4][4]) ;
void
  _dxfAdjointTranspose (register double s0[4][4], register double s1[4][4]);
void
  _dxfTranspose (register double s0[4][4], register double s1[4][4]);
void
  _dxfRenormalizeView (register float m[4][4]) ;
void
  _dxfGetNearFar(int projection, int resolution, float width,
		float from[3], float zaxis[3], float box[8][3],
		float *Near, float *Far) ;

/* Convert a dx style matrix to a float[4][4] tdm matrix */
#define CONVERTMATRIX(tdmM, dxM) \
  tdmM[0][0]=dxM.A[0][0]; \
  tdmM[0][1]=dxM.A[0][1]; \
  tdmM[0][2]=dxM.A[0][2]; \
  tdmM[0][3]=0.0;	  \
\
  tdmM[1][0]=dxM.A[1][0]; \
  tdmM[1][1]=dxM.A[1][1]; \
  tdmM[1][2]=dxM.A[1][2]; \
  tdmM[1][3]=0.0;	  \
\
  tdmM[2][0]=dxM.A[2][0]; \
  tdmM[2][1]=dxM.A[2][1]; \
  tdmM[2][2]=dxM.A[2][2]; \
  tdmM[2][3]=0.0;	  \
\
  tdmM[3][0]=dxM.b[0];	  \
  tdmM[3][1]=dxM.b[1];	  \
  tdmM[3][2]=dxM.b[2];	  \
  tdmM[3][3]=1.0


/* copy matrices m1 <- m2 (Where either can be float or double) */
#define COPYMATRIX(m1, m2)                         \
{                                                  \
  int iii,jjj; 					   \
  for(iii=0;iii<4;iii++) 				\
    for(jjj=0;jjj<4;jjj++) 				\
     (m1)[iii][jjj] = (m2)[iii][jjj]; 		      	\
}

/* multiply single precision matrices m1 <- m2 x m3 */
#define MULTMATRIX(m1, m2, m3)     \
{                                  \
 register float (*p1)[4] = m1 ;    \
 register float (*p2)[4] = m2 ;    \
 register float (*p3)[4] = m3 ;    \
 p1[0][0]  = p2[0][0] * p3[0][0] ; \
 p1[0][0] += p2[0][1] * p3[1][0] ; \
 p1[0][0] += p2[0][2] * p3[2][0] ; \
 p1[0][0] += p2[0][3] * p3[3][0] ; \
 p1[0][1]  = p2[0][0] * p3[0][1] ; \
 p1[0][1] += p2[0][1] * p3[1][1] ; \
 p1[0][1] += p2[0][2] * p3[2][1] ; \
 p1[0][1] += p2[0][3] * p3[3][1] ; \
 p1[0][2]  = p2[0][0] * p3[0][2] ; \
 p1[0][2] += p2[0][1] * p3[1][2] ; \
 p1[0][2] += p2[0][2] * p3[2][2] ; \
 p1[0][2] += p2[0][3] * p3[3][2] ; \
 p1[0][3]  = p2[0][0] * p3[0][3] ; \
 p1[0][3] += p2[0][1] * p3[1][3] ; \
 p1[0][3] += p2[0][2] * p3[2][3] ; \
 p1[0][3] += p2[0][3] * p3[3][3] ; \
 p1[1][0]  = p2[1][0] * p3[0][0] ; \
 p1[1][0] += p2[1][1] * p3[1][0] ; \
 p1[1][0] += p2[1][2] * p3[2][0] ; \
 p1[1][0] += p2[1][3] * p3[3][0] ; \
 p1[1][1]  = p2[1][0] * p3[0][1] ; \
 p1[1][1] += p2[1][1] * p3[1][1] ; \
 p1[1][1] += p2[1][2] * p3[2][1] ; \
 p1[1][1] += p2[1][3] * p3[3][1] ; \
 p1[1][2]  = p2[1][0] * p3[0][2] ; \
 p1[1][2] += p2[1][1] * p3[1][2] ; \
 p1[1][2] += p2[1][2] * p3[2][2] ; \
 p1[1][2] += p2[1][3] * p3[3][2] ; \
 p1[1][3]  = p2[1][0] * p3[0][3] ; \
 p1[1][3] += p2[1][1] * p3[1][3] ; \
 p1[1][3] += p2[1][2] * p3[2][3] ; \
 p1[1][3] += p2[1][3] * p3[3][3] ; \
 p1[2][0]  = p2[2][0] * p3[0][0] ; \
 p1[2][0] += p2[2][1] * p3[1][0] ; \
 p1[2][0] += p2[2][2] * p3[2][0] ; \
 p1[2][0] += p2[2][3] * p3[3][0] ; \
 p1[2][1]  = p2[2][0] * p3[0][1] ; \
 p1[2][1] += p2[2][1] * p3[1][1] ; \
 p1[2][1] += p2[2][2] * p3[2][1] ; \
 p1[2][1] += p2[2][3] * p3[3][1] ; \
 p1[2][2]  = p2[2][0] * p3[0][2] ; \
 p1[2][2] += p2[2][1] * p3[1][2] ; \
 p1[2][2] += p2[2][2] * p3[2][2] ; \
 p1[2][2] += p2[2][3] * p3[3][2] ; \
 p1[2][3]  = p2[2][0] * p3[0][3] ; \
 p1[2][3] += p2[2][1] * p3[1][3] ; \
 p1[2][3] += p2[2][2] * p3[2][3] ; \
 p1[2][3] += p2[2][3] * p3[3][3] ; \
 p1[3][0]  = p2[3][0] * p3[0][0] ; \
 p1[3][0] += p2[3][1] * p3[1][0] ; \
 p1[3][0] += p2[3][2] * p3[2][0] ; \
 p1[3][0] += p2[3][3] * p3[3][0] ; \
 p1[3][1]  = p2[3][0] * p3[0][1] ; \
 p1[3][1] += p2[3][1] * p3[1][1] ; \
 p1[3][1] += p2[3][2] * p3[2][1] ; \
 p1[3][1] += p2[3][3] * p3[3][1] ; \
 p1[3][2]  = p2[3][0] * p3[0][2] ; \
 p1[3][2] += p2[3][1] * p3[1][2] ; \
 p1[3][2] += p2[3][2] * p3[2][2] ; \
 p1[3][2] += p2[3][3] * p3[3][2] ; \
 p1[3][3]  = p2[3][0] * p3[0][3] ; \
 p1[3][3] += p2[3][1] * p3[1][3] ; \
 p1[3][3] += p2[3][2] * p3[2][3] ; \
 p1[3][3] += p2[3][3] * p3[3][3] ; \
}

/*
 * APPLY applies the matrix p <- p X m (m either float or double)
 */
#define APPLY4(p,m) \
{ float t[4]; \
t[0]=(p)[0] * (m)[0][0] + (p)[1] * (m)[1][0] + (p)[2] * (m)[2][0] + (m)[3][0]; \
t[1]=(p)[0] * (m)[0][1] + (p)[1] * (m)[1][1] + (p)[2] * (m)[2][1] + (m)[3][1]; \
t[2]=(p)[0] * (m)[0][2] + (p)[1] * (m)[1][2] + (p)[2] * (m)[2][2] + (m)[3][2]; \
t[3]=(p)[0] * (m)[0][3] + (p)[1] * (m)[1][3] + (p)[2] * (m)[2][3] + (m)[3][3]; \
(p)[0] = t[0]; (p)[1] = t[1]; (p)[2] = t[2]; (p)[3] = t[3]; \
}
#define APPLY3(p,m) \
{ float t[3]; \
t[0]=(p)[0] * (m)[0][0] + (p)[1] * (m)[1][0] + (p)[2] * (m)[2][0] + (m)[3][0]; \
t[1]=(p)[0] * (m)[0][1] + (p)[1] * (m)[1][1] + (p)[2] * (m)[2][1] + (m)[3][1]; \
t[2]=(p)[0] * (m)[0][2] + (p)[1] * (m)[1][2] + (p)[2] * (m)[2][2] + (m)[3][2]; \
(p)[0] = t[0]; (p)[1] = t[1]; (p)[2] = t[2]; \
}

/*
 *  ROTXY creates a matrix which rotates about a unit vector [x y 0].
 *  This matrix can be composed from a rotation about Z to align the
 *  vector with the X axis, followed by a rotation about the X axis, then
 *  finished with the inverse of the Z rotation:
 *
 *  x -y  0  0       1      0       0   0        x  y  0  0
 *  y  x  0  0   X   0  cos(a)  sin(a)  0   X   -y  x  0  0
 *  0  0  1  0       0 -sin(a)  cos(a)  0        0  0  1  0
 *  0  0  0  1       0      0       0   1        0  0  0  1
 *
 *  This composes into:
 *
 *  x*x + y*y*cos(a)  x*y - x*y*cos(a)  -y*sin(a)  0
 *  x*y - x*y*cos(a)  y*y + x*x*cos(a)   x*sin(a)  0
 *          y*sin(a)         -x*sin(a)     cos(a)  0
 *                0                 0          0   1
 *
 *  By using the identity x*x + y*y = 1 and defining t = 1 - cos(a), the
 *  matrix can be simplified as follows:
 */ 

#define ROTXY(m, x, y, s, c, t) {\
m[0][0]= t*x*x + c;   m[0][1]=  t*x*y;       m[0][2]= -s*y;         m[0][3] =0;\
m[1][0]= t*x*y;       m[1][1]=  t*y*y + c;   m[1][2]=  s*x;         m[1][3] =0;\
m[2][0]= s*y;         m[2][1]= -s*x;         m[2][2]=  c;           m[2][3] =0;\
m[3][0]= 0;           m[3][1]=  0;           m[3][2]=  0;           m[3][3] =1;}

/*
 *  ROTXYZ is derived in a fashion analogous to ROTXY, extended for the Z
 *  component of a unit rotation axis [x y z].
 */

#define ROTXYZ(m, x, y, z, s, c, t) {\
m[0][0]= t*x*x + c;   m[0][1]= t*x*y + s*z; m[0][2]= t*x*z - s*y; m[0][3]= 0;\
m[1][0]= t*x*y - s*z; m[1][1]= t*y*y + c;   m[1][2]= t*y*z + s*x; m[1][3]= 0;\
m[2][0]= t*x*z + s*y; m[2][1]= t*y*z - s*x; m[2][2]= t*z*z + c;   m[2][3]= 0;\
m[3][0]= 0;           m[3][1]= 0;           m[3][2]= 0;           m[3][3]= 1;}

/*
 *  Obtain the view coordinate system origin from an isotropic view matrix
 *  by transforming point [M41 M42 M43] by the transpose of the view rotation.
 */

#define GET_VC_ORIGIN(m, p) {\
p[0] = -(m[3][0]*m[0][0] + m[3][1]*m[0][1] + m[3][2]*m[0][2]) ;\
p[1] = -(m[3][0]*m[1][0] + m[3][1]*m[1][1] + m[3][2]*m[1][2]) ;\
p[2] = -(m[3][0]*m[2][0] + m[3][1]*m[2][1] + m[3][2]*m[2][2]) ;}

/*
 *  Get the view direction from a view matrix.
 */

#define GET_VIEW_DIRECTION(m, z) {\
z[0] = -m[0][2] ; z[1] = -m[1][2] ; z[2] = -m[2][2] ; } 
  				
static const float identity[4][4] =
{
  {1.0, 0.0, 0.0, 0.0},
  {0.0, 1.0, 0.0, 0.0},
  {0.0, 0.0, 1.0, 0.0},
  {0.0, 0.0, 0.0, 1.0}
};

static const double dIdentity[4][4] =
{
  {1.0, 0.0, 0.0, 0.0},
  {0.0, 1.0, 0.0, 0.0},
  {0.0, 0.0, 1.0, 0.0},
  {0.0, 0.0, 0.0, 1.0}
};

static const double dZero[4][4] =
{
  {0.0, 0.0, 0.0, 0.0},
  {0.0, 0.0, 0.0, 0.0},
  {0.0, 0.0, 0.0, 0.0},
  {0.0, 0.0, 0.0, 0.0}
};

#endif /* tdmMatrix_h */
