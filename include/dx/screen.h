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

#ifndef _DXI_SCREEN_H_
#define _DXI_SCREEN_H_

/* TeX starts here. Do not remove this comment. */

/*
\section{Screen class}

A screen object specifies an object that maintains a size and
alignment with the screen (output image) independent of the camera
view and scaling transformations applied to the screen object.

Three options are provided for the interpretation of translations
applied to a screen object.  First, a translation applied to the
screen object may specify a new position for the origin of the screen
object in world space.  Second, a translation applied to the screen
object may specify a new location for the screen object in the image,
measured in pixels, where (0,0) refers to the lower-left corner of the
image.  Third, a translation applied to the screen object may specify
a new location for the screen object in image, measured in
viewport-relative coordinates, where (0,0) refers to the lower-left
corner of the image and (1,1) refers to the upper-right corner of the
image.

In addition, with regard to $z$, the object may be displayed either in
place in the scene, in front of all objects, or behind all objects,
according to whether the {\tt z} parameter to {\tt DXNewScreen()} is 0,
$+1$, or $-1$ respectively.
*/

#define SCREEN_WORLD 0
#define SCREEN_VIEWPORT 1
#define SCREEN_PIXEL 2
#define SCREEN_STATIONARY 3

Screen DXNewScreen(Object o, int position, int z);
/**
\index{DXNewScreen}
Creates a new object representing {\tt o} aligned to the final screen.
The {\tt position} parameter specifies one of the three options for
positioning of the screen object discussed above; it must be one of
{\tt SCREEN\_WORLD}, {\tt SCREEN\_PIXEL}, or {\tt SCREEN\_VIEWPORT}.
The {\tt z} parameter determines the relative depth of the object, as
described above.  Returns the screen object, or returns null and sets
the error code to indicate an error.
**/

Screen DXGetScreenInfo(Screen s, Object *o, int *position, int *z);
/**
\index{DXGetScreenInfo}
If {\tt o} is not null, returns the object to be transformed in {\tt
*o}.  If {\tt position} is not null, returns in {\tt *position} the
positioning option specified when the screen object was created.  If
{\tt z} is not null, returns in {\tt *z} the {\tt z} option specified
when the screen object was created.  Returns {\tt s}, or returns null
and sets the error code to indicate an error.
**/

Screen DXSetScreenObject(Screen s, Object o);
/**
\index{DXSetScreenObject}
This routine sets the object to which the screen transform is applied
to {\tt o}.  Returns {\tt s}, or returns null and sets the error code
to indicate an error.
**/

#endif /* _DXI_SCREEN_H_ */

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif
