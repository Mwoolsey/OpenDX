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

#ifndef _DXI_CAMERA_H_
#define _DXI_CAMERA_H_

/* TeX starts here. Do not remove this comment. */

/*
\section{Camera class}
\label{camsec}
A camera object stores parameters that relate a scene to an image of
the scene, including camera position and orientation, field of view,
type of projection, and resolution.  It specifies the transformation
from world coordinates to image coordinates.  See the description of
the {\tt DXTransform()} routine in Section \ref{transformsec}.
*/

Camera DXNewCamera(void);
/**
\index{DXNewCamera}
Creates a new camera.  The camera is initialized with an orthographic
projection looking along the $z$ axis toward the origin, with $x$ and
$y$ each ranging from $-1$ to $+1$ visible in a square viewport, with
the origin at the center of the viewport.  Returns the camera, or
returns null and sets the error code to indicate an error.
**/

Camera DXSetView(Camera c, Point from, Point to, Vector up);
/**
\index{DXSetView}
Specifies the camera position ({\tt from}), a point on the line of
sight of the camera ({\tt to}), and the camera orientation ({\tt up}).
Returns {\tt c}, or returns null and sets the error code to indicate
an error.
**/

Camera DXSetOrthographic(Camera c, double width, double aspect);
/**
\index{DXSetOrthographic}
Specifies an orthographic view.  The width of the viewport in world
coordinates is given by {\tt width}; the height of the view is {\tt
aspect} times the width.  Returns {\tt c}, or returns null and sets
the error code to indicate an error.
**/

Camera DXSetPerspective(Camera c, double width, double aspect);

Camera DXSetResolution(Camera c, int hres, double pix_aspect);
/**
\index{DXSetResolution}
Specifies the resolution of the camera.  The horizontal resolution in
pixels is given by {\tt hres}.  The pixels are assumed to be {\tt
pix\_aspect} times as tall as they are wide.  The vertical resolution
in pixels is {\tt hres*aspect/pix\_aspect}, where {\tt aspect} was
specified by {\tt DXSetPerspective()} or {\tt DXSetOrthographic()}.
Returns {\tt c}, or returns null and sets the error code to indicate
an error.
**/

Matrix DXGetCameraMatrix(Camera c);
Matrix DXGetCameraRotation(Camera c);
Matrix DXGetCameraMatrixWithFuzz(Camera c, float fuzz);
/**
\index{DXGetCameraMatrix}\index{DXGetCameraRotation}
The {\tt DXGetCameraMatrix()} returns the transformation matrix defined
by camera {\tt c}.  The {\tt DXGetCameraRotation()} routine returns only
the rotation part of the camera transformation.
**/

Camera DXGetView(Camera c, Point *from, Point *to, Vector *up);
/**
\index{DXGetView}
If {\tt from} is not null, this routine returns the camera from
position in {\tt *from}.  If {\tt to} is not null, it returns the
camera to position in {\tt *to}.  If {\tt up} is not null, it returns
the camera up vector in {\tt *up}.  Returns {\tt c}, or returns null
and sets the error code to indicate an error.
**/

Camera DXGetCameraResolution(Camera c, int *width, int *height);
/**
\index{DXGetCameraResolution}
Gets the resolution associated with camera {\tt c}.  Returns {\tt c},
or returns null and sets the error code to indicate an error.
**/

Camera DXGetOrthographic(Camera c, float *width, float *aspect);
/**
\index{DXGetOrthographic}
If {\tt width} is not null, returns the width of view in {\tt
*fov}.  If {\tt aspect} is not null, returns the aspect ratio in
{\tt *aspect}.  Returns {\tt c} if it is an orthographic camera,
otherwise returns null but does not set the error code.
**/

Camera DXGetPerspective(Camera c, float *fov, float *aspect);

Camera DXGetBackgroundColor(Camera camera, RGBColor *background);
Camera DXSetBackgroundColor(Camera camera, RGBColor background);

#endif /* _DXI_CAMERA_H_ */

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif
