/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

#ifndef _DXI_USERINTERACTORS_H_
#define _DXI_USERINTERACTORS_H_

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


typedef struct
{
    int   event;
    int   pad[7];
} DXAnyEvent;

typedef struct
{ 
    int   event;
    int   x;
    int   y;
    int   state;
    int   kstate;
} DXMouseEvent;

typedef struct
{ 
    int   event;
    int   x;
    int   y;
    int   key;
    int   kstate;
} DXKeyPressEvent;

typedef union 
{
	DXAnyEvent	any;
	DXMouseEvent 	mouse;
	DXKeyPressEvent keypress;
} DXEvent,  *DXEvents;

/*
 * The following are convenience typedefs that MUST agree with the 
 * subsequent structure definition elements
 */
typedef void *(*InitMode)(Object args, int width, int height, int *mask);
typedef void  (*EndMode)(void *handle);
typedef void  (*SetCamera)(void *handle, float *to, float *from, float *up,
                              int projection, float fov, float width);
typedef int   (*GetCamera)(void *handle, float *to, float *from, float *up,
                              int *projection, float *fov, float *width);
typedef void  (*SetRenderable)(void *handle, Object obj);
typedef int   (*GetRenderable)(void *handle, Object *obj);

typedef void  (*EventHandler)(void *handler, DXEvent *event);

struct _userInteractor
{
    void *(*InitMode)(Object, int, int, int *);
    void  (*EndMode)(void *);
    void  (*SetCamera)(void *, float *, float *, float *,
		int, float, float);
    int   (*GetCamera)(void *, float *, float *, float *,
		int *, float *, float *);
    void  (*SetRenderable)(void *, Object);
    int   (*GetRenderable)(void *, Object *);

    void  (*EventHandler)(void *, DXEvent *);
};

typedef struct _userInteractor UserInteractor;

/*
 * Events
 */
#define DXEVENT_LEFT		0x01
#define DXEVENT_MIDDLE		0x02
#define DXEVENT_RIGHT		0x04
#define DXEVENT_KEYPRESS	0x08

/*
 * Button states
 */
#define BUTTON_UP     1
#define BUTTON_DOWN   2
#define BUTTON_MOTION 3

#define LEFTBUTTON	0
#define MIDDLEBUTTON	1
#define RIGHTBUTTON	2
#define NO_BUTTON	-1

#define WHICH_BUTTON(e)	\
	((((DXAnyEvent *)e)->event) == DXEVENT_LEFT ? LEFTBUTTON :				\
	    (((DXAnyEvent *)e)->event) == DXEVENT_MIDDLE ? MIDDLEBUTTON :			\
		(((DXAnyEvent *)e)->event) == DXEVENT_RIGHT ? RIGHTBUTTON : NO_BUTTON)

#endif /* _DXI_USERINTERACTORS_H_ */

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif

