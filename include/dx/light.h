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

#ifndef _DXI_LIGHT_H_
#define _DXI_LIGHT_H_

/* TeX starts here. Do not remove this comment. */

/*
\section{Light class}
\label{lightsec}
Light objects specify lights for shading.  They may be placed in a
scene and transformed along with other objects in the scene, changing
for example their position.
*/

Light DXNewDistantLight(Vector direction, RGBColor color);
Light DXNewCameraDistantLight(Vector direction, RGBColor color);
/**
\index{DXNewDistantLight}
Creates a light object representing a point light source at infinity
in the specified direction.  Returns the light, or returns null and
sets the error code to indicate an error.
**/

Light DXQueryDistantLight(Light l, Vector *direction, RGBColor *color);
Light DXQueryCameraDistantLight(Light l, Vector *direction, RGBColor *color);
/**
\index{DXQueryDistantLight}
Determines whether {\tt l} is a distant light and returns the
information specified when the light was created in {\tt *direction}
and {\tt *color}.  Returns {\tt l}, or returns null but does not set
the error code if {\tt l} is not a distant light.
**/

Light DXNewAmbientLight(RGBColor color);
/**
\index{DXNewAmbientLight}
Creates a light object representing an ambient light source.  Returns
the light, or returns null and sets the error code to indicate an
error.
**/

Light DXQueryAmbientLight(Light l, RGBColor *color);
/**
\index{DXQueryAmbientLight}
Determines whether {\tt l} is an ambient light and returns the
information specified when the light was created in {\tt *color}.
Returns {\tt l}, or returns null but does not set the error code if
{\tt l} is not a ambient light.
**/

#endif /* _DXI_LIGHT_H_ */

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif
