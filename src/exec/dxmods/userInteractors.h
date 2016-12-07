/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/
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

#include <dxconfig.h>

#ifndef _USERINTERACTORS_H_
#define _USERINTERACTORS_H_

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

typedef void *(*CameraInitModeP)();

typedef void  (*CameraStartStrokeP)(void *, int, int, int,
                              float *, float *, float *,
                              float, int, float, float,
                              int, int);

typedef void  (*CameraStrokePointP)(void *, int, int,
                              float *, float *, float *,
                              float *, int *, float *, float *);

typedef void  (*CameraEndStrokeP)(void *,
                              float *, float *, float *,
                              float *, int *, float *, float *);

typedef void (*CameraEndModeP)(void *);


struct _cameraInteractor
{
    CameraInitModeP    InitMode;
    CameraStartStrokeP StartStroke;
    CameraStrokePointP StrokePoint;
    CameraEndStrokeP   EndStroke;
    CameraEndModeP     EndMode;
};

typedef struct _cameraInteractor CameraInteractor;

typedef void *(*ObjectInitModeP)(Object);
typedef void  (*ObjectStartStrokeP)(void *, int, int, int,
                              float *, float *, float *,
                              int, float, float,
                              int, int, Object, Object *);
typedef void  (*ObjectStrokePointP)(void *, int, int, Object, Object *);
typedef void  (*ObjectEndStrokeP)(void *, Object, Object *);
typedef void (*ObjectEndModeP)(void *);


struct _objectInteractor
{
    ObjectInitModeP    InitMode;
    ObjectStartStrokeP StartStroke;
    ObjectStrokePointP StrokePoint;
    ObjectEndStrokeP   EndStroke;
    ObjectEndModeP     EndMode;
};

typedef struct _objectInteractor ObjectInteractor;

#endif /* _USERINTERACTORS_H_ */
