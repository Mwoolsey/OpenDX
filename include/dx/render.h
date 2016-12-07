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

#ifndef _DXI_RENDER_H_
#define _DXI_RENDER_H_

/* TeX starts here. Do not remove this comment. */

/*

This chapter describes the Data Explorer rendering model in more
detail, introduces some additional elements of the data model that are
relevant only to rendering, and describes routines for manipulating
those data structures and for rendering.

The Data Explorer renderer is designed around scientific visualization
requirements.  Thus, for example, it directly renders scenes described
by the Data Explorer data model described in previous chapters.  The
renderer handles all combinations of groups and fields as input
objects.  The members of any group or subclass of group (e.g. series and
composite field) are combined into one image by the renderer.

Rendering a scene has four steps: transformation to world coordinates,
shading, transformation to image coordinates, and tiling.
Transformation to world coordinates applies transformations specified
by {\tt Xform} nodes in the object.  The shading step assigns colors
to the vertices using the intrinsic surface colors, surface normals,
surface properties specified by field components, and lights specified
by {\tt Light} objects.  Transformation to image coordinates is
specified by a {\tt Camera} object.  The tiling step generates the
image by linearly interpolating point colors and opacities across
faces, and rendering volumes by using one of a variety of irregular
and regular volume rendering algorithms.  Rendering can be
accomplished by the {\tt DXRender()} routine:
*/

Field DXRender(Object o, Camera c, char *format);
/**
\index{DXRender}
Renders {\tt o} using the parameters defined by attributes of the
object's fields and using the camera defined by {\tt c}. It returns a
new image field containing the result.  This performs the
transformation, shading, and the tiling steps.  If {\tt format} is
specified as null, a generic floating point image is used; this is the
most flexible format with respect to processing by other modules.
Alternatively, {\tt format} may be specified as a character string
identifying a harware-specific format that may be used only for
display on a particular hardware device.  Returns the image, or
returns null and sets the error code to indicate an error.
\medskip
**/

/*
\paragraph{Color, opacity and normal dependencies.}

Colors, opacities and normals may be dependent on the positions (when
``colors'', ``opacities'' and ``normals'' components have a ``dep''
attribute of ``positions'') or they may be dependent on connections
(when ``colors'', ``opacities'' and ``normals'' components have a
``dep'' attribute of ``connections'').

If opacities or normals are present, they must depend on the same
component that the colors depend upon, with one exception: if the
colors are dependent on the positions and the normals are dependent on
the connections, the face will be flat-shaded with a color that is the
average color of the face vertices.

If the colors, opacities and normals are dependent on the positions,
the color and opacity of each face is linearly interpolated between
the vertices (Gouraud shading). If the colors, opacities and normals
are dependent on the connections, the color and opacity of each face
is constant (flat shading).  The following table summarizes, for the
current release of the Data Explorer, which dependencies of colors,
opacities, and normals are supported.

\begin{center}
\newcommand{\no}{\notimplementedsymbol}
\begin{tabular}{l|l|l|l}
dependency      & ``colors''   & ``opacities''  & ``normals''  \\
\hline
``positions''   & yes	       & yes            & yes          \\
\hline
``lines''       & yes          & ---            & ---          \\
``triangles''   & yes          & yes            & yes*         \\
``quads''       & yes          & yes            & yes*         \\
``tetrahedra''  & \no          & \no            & yes*         \\
``cubes''       & \no          & \no            & yes*         \\
``faces''       & yes          & ---            & yes*
\end{tabular}
\end{center}
In this table, ``yes'' means implemented, ``\notimplementedsymbol'' means
not yet implemented, and ``---'' means not meaningful or irrelevant due to
implementation restrictions in other areas of the renderer.  The * indicates
that shading of normals that are dependent upon connections is only implemented
for distant and ambient light sources.

\section{Transformation}
\label{transformsec}

Transformation is the process of computing (ultimately) pixel
coordinates from model coordinates.  The {\tt DXRender()} function
performs necessary transformations, so the {\tt DXTransform()} function
is not needed by most applications.

Transformation can be thought of as having two steps: transforming
from model coordinates to world coordinates, and transforming from
world coordinates to image coordinates:
\begin{center}
\makebox[0pt]{\psfig{figure=coord.px}}
\end{center}
The transformation from model to world coordinates is specified by
xform nodes (see Section \ref{xformsec}) in the description of the
input object.  The transformation from world coordinates to image
coordinates is specified by a camera object (see Section \ref{camsec})
provided as an argument to the {\tt DXTransform()} routine.
*/

/*
\section{Surface Shading}

Shading is the process of applying lights to a surface according to
shading parameters.  The shading process described here is performed
by the {\tt DXRender()} function for surfaces objects only; volumes are
rendered directly using the colors and opacities specified.  The
lights are specified by light objects (see Section \ref{lightsec})
contained in the input object.  The shading process uses the following
field components:

\begin{center}
\begin{tabular}{l|l}
Component        & Meaning \\
\hline
``positions''    & points \\
``colors''       & front and back colors \\
``front colors'' & colors of front of face \\
``back colors''  & colors of back of face \\
``normals''      & surface normals
\end{tabular}
\end{center}
A field may have both ``colors'' and ``front colors'' or both ``colors''
and ``back colors'', in which case the ``front colors'' or ``back colors''
component overrides the ``colors'' component for the specified side of the
object.  Shading parameters are specified by a set of attributes of an
input object.  The attributes are:
\begin{center}
\begin{tabular}{l|l}
Attribute       & Meaning \\
\hline
``ambient''     & ambient lighting coefficient $k_a$ \\
``diffuse''     & diffuse lighting coefficient $k_d$ \\
``specular''    & specular lighting coefficient $k_s$ \\
``shininess''   & specular lighting exponent $e$
\end{tabular}
\end{center}
The parameters listed above apply to both the front and back of an object.
In addition, for each parameter ``$x$'', there is also a ``front $x$'' and
a ``back $x$'' parameter that applies only to the front and back of a surface
respectively. These parameters are used in the following shading model:
\[
I = k_a A C + k_d L C {\bf n}\cdot{\bf l} + k_s L ({\bf n}\cdot{\bf h})^e
\]
where $I$ is apparent intensity of the object, $A$ is an ambient light,
$L$ is a point or distant light, $C$ is the color of the object,
${\bf n}$ is the surface normal, ${\bf l}$ is the direction to the light,
and ${\bf h}$ is a unit vector halfway between the direction to the camera
and the direction to the light.
*/

/*
\section{Tiling}

Tiling is the process of combining shaded surface and volume
interpolation elements to produce an image.  The following table lists
the supported interpolation elements:
\begin{center}
\newcommand{\no}{\notimplementedsymbol}
\begin{tabular}{l||c|c||c|c}
Component       & irregular & regular & opaque & translucent \\
\hline
``lines''       & yes       & yes     & yes    & yes           \\
``triangles''   & yes       & ---     & yes    & yes         \\
``quads''       & yes       & yes     & yes    & yes         \\
``tetrahedra''  & yes       & ---     & ---    & yes         \\
``cubes''       & yes       & yes     & ---    & yes         \\
``faces,'' ``loops,'' ``edges'' & yes & --- & yes & \no \\
\end{tabular}
\end{center}
In this table, ``yes'' means defined and implemented,
``\notimplementedsymbol'' means defined but not implemented in the
current release of the Data Explorer, and ``---'' indicates a
meaningless combination.

Lines may be irregular unconnected vectors, or {\em paths} having
regular one-dimensional connections.  Surfaces and volumes may be
completely irregular, regular in connections but irregular in
positions, or regular in both connections and positions.  The
following table illustrates the six classes.
\newcommand{\foo}[2]{\begin{minipage}[b]{1.9in}\begin{tabular}{ll}
    connections & #1\\
    positions & #2
\end{tabular}\end{minipage}}
\begin{center}
\begin{tabular}{c@{\hspace{.4in}}c@{\hspace{.4in}}|c}
\\
Surfaces & Volumes & Associated connections components \\[5pt]
\hline&&\\
\psfig{figure=mesh1.px} & \psfig{figure=mesh4.px} &
\foo{irregular {\tt Array}}{irregular {\tt Array}} \\
\psfig{figure=mesh2.px} & \psfig{figure=mesh5.px} &
\foo{regular connections {\tt Array}}{irregular {\tt Array}} \\
\psfig{figure=mesh3.px} & \psfig{figure=mesh6.px} &
\foo{regular connections {\tt Array}}{regular positions {\tt Array}}
\end{tabular}
\end{center}

\paragraph{Rendering model}.
The interpretation of ``colors'' and ``opacities'' differs
between surfaces and volumes.  For surfaces, a surface of color $c_f$
and opacity $o$ is combined with the color $c_b$ of the objects behind
it resulting in a combined color $c_f o + c_b (1-o)$.

For volumes, the ``dense emitter'' model is used, in which the opacity
represents the instantaneous rate of absorption of light passing
through the volume per unit thickness, and the color represents the
instanteous rate of light emission per unit thickness.  If $c(z)$
represents the color of the object at $z$ and $o(z)$ represents its
opacity at $z$, then the total color of a ray passing through the
volume is given by
\[
    c =  \int_{-\infty}^\infty c(z)
	\exp \left( - \int_{-\infty}^z o(\zeta)\,d\zeta \right)\,dz.
\]



\paragraph{Tiling options}
Tiling options are controlled by a set of object attributes.  These
attributes may be associated with objects at any level of a
field/group hierarchy.  The attributes may be set by using the {\tt
DXSetAttribute()} function, or by using the Options module.  The current
attributes are:
\begin{center}
\begin{tabular}{l|l}
Attribute       & Meaning \\
\hline
``fuzz''	& object fuzz \\
\end{tabular}
\end{center}
Object fuzz is a method of resolving conflicts between objects at the
same distance from the camera.  For example, it may be desirable to
define a set of lines coincident with a plane.  Normally it will be
ambiguous which object is to be displayed in front.  In addition,
single-pixel lines are inherently inaccurate (deviate from the actual
geometric line) by as much as one-half pixel; when displayed against a
sloping surface, this $x$ or $y$ inaccuracy is equivalent to a $z$
inaccuracy related to the slope of the surface.  The ``fuzz''
attribute specifies a $z$ value that will be added to the object
before it is compared with other objects in the scene, thus resolving
this problem.  The fuzz value is specified in units of pixels.  Thus,
for example, a fuzz value of one pixel is able to compensate for the
half-pixel line inaccuracy described above when the line is displayed
against a surface with a slope of two.
*/

#endif /* _DXI_RENDER_H_ */

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif
