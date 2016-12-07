/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/
/*********************************************************************/
/*                     I.B.M. CONFIENTIAL                           */
/*********************************************************************/

#include <dxconfig.h>


#ifndef sbutils_c_h
#define sbutils_c_h
/* SBUTILS:sbutils.c.h    07/14/90    01:44:21 */

/* ANSI C uses function prototypes for passing floats */

#ifdef __STDC__ /* FN PROTO [ */
int translate3d(float xt, float yt, float zt, int stack);
int rotate3d(float xa, float ya, float za, int stack);
int scale3d(float xs, float ys, float zs, int stack);

#else /* __STDC__ */

/* These alias's are needed for K&R C */

#define translate3d c_translate3d
#define scale3d c_scale3d
#define rotate3d c_rotate3d

#endif /* __STDC__ */

/* These macros define how flexible a program is when it requests a window's
 * creation with either the CreateImagePlanesWindow() or
 * CreateOverlayPlanesWindow():
 */
#define NOT_FLEXIBLE            0
#define FLEXIBLE                1


/* These macros define the values of the "sbCmapHint" parameter of the
 * CreateImagePlanesWindow():
 */
#ifndef SB_CMAP_TYPE_NORMAL
#define SB_CMAP_TYPE_NORMAL     1
#endif

#ifndef SB_CMAP_TYPE_MONOTONIC
#define SB_CMAP_TYPE_MONOTONIC  2
#endif

#ifndef SB_CMAP_TYPE_FULL
#define SB_CMAP_TYPE_FULL       4
#endif

#endif /*  sbutils_c_h */
