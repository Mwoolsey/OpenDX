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

#ifndef _DXI_GEOMETRY_H_
#define _DXI_GEOMETRY_H_

/* TeX starts here. Do not remove this comment. */

/*
\section{Text}
Two varieties of text are supported by the system.  Geometric text is
implemented by generating a geometric surface like a paper cutout of
the text, which can then be arbitrarily rotated and scaled before
rendering.  Annotation text is text that is transformed so that it
always faces the screen.  This is supported by a combination of the
geometric text primitive described in this section and the screen
object described Chapter \ref{rendchap}.
*/

Object DXGetFont(char *name, float *ascent, float *descent);
/**
\index{DXGetFont}
Returns a group representing the named font.  The group has as many
members as there are characters in the font.  The character {\tt 'c'}
is represented by the {\tt 'c'}th member, and can be obtained by {\tt
DXGetEnumeratedMember(font, 'c', NULL)}.  The font has an overall height
of 1.  If {\tt ascent} is not null, the portion of the overall height
above the baseline is returned in {\tt *ascent}.  If {\tt descent} is
not null, the portion of the overall height below the baseline is
returned in {\tt *descent}.  The sum of the ascent and the descent is
the overall height 1.  Returns the font, or returns null and sets the
error code to indicate an error.
**/

Object DXGeometricText(char *s, Object font, float *width);
/**
\index{DXGeometricText}
Produces an object consisting of the given string.  The font is
specified by an object returned by {\tt DXGetFont()}.  The origin of the
string (left end of baseline) is placed at the origin of the $x,y$
plane with the baseline pointed along the positive $x$ axis.  If {\tt
width} is not null, the width of the string will be returned in {\tt
*width}.  The string will be bounded above by {\tt ascent} and below
by {\tt -descent} (as returned by {\tt DXGetFont()}), to the left by 0,
and to the right by {\tt width}.  Returns the text object, or returns
null and sets the error code to indicate an error.
**/

/*
\section{Clipping}

This section describes two higher-level routines that use the DXRender
module's clipping capability to clip an object to a plane and to a
box.  These routines do not actually perform any clipping, but rather
construct an object (clipped object) that describes to the renderer
what clipping is to be done at render time.
*/

Object DXClipPlane(Object o, Point p, Vector n);
/**
\index{DXClipPlane}
Clips object {\tt o} by the plane that contains {\tt p} and is
perpendicular to {\tt n}.  The object on the side of the plane pointed
to by {\tt n} is retained.  Returns an object describing to the
renderer how to do the clipping at render time, or returns null and
sets the error code to indicate an error.
**/

Object DXClipBox(Object o, Point p1, Point p2);
/**
\index{DXClipBox}
Clips object {\tt o} by the box defined by points {\tt p1} and {\tt
p2}.  Returns an object describing to the renderer how to do the
clipping at render time, or returns null and sets the error code to
indicate an error.
**/

/*
\section{Path operations}

The following operations produce a geometric object from a path.  In
addition to the functions described here, the DXRender module is capable
of directly rendering a path as a series of one-pixel lines.  A path
is a field with one-dimensional regular connections.  A path can be
created by, for example:

\begin{program}
    f = DXNewField();
    DXSetComponentValue(f, "positions", ...);
    DXSetConnections(f, "lines", DXMakeGridConnections(1, n));
    DXEndField(f);
\end{program}
where {\tt n} is the number of points.

Both of the operations described here use ``normals'' and ``tangent''
components if they are present; otherwise, they compute approximations
to the normals and tangents, as follows: the tangent is the first
derivative of the path, and the normal is perpendicular to the tangent
and in the plane formed by the tangent and the second derivative of
the path.  In each case, appropriate normals are associated with the
result for shading.
*/

Object DXRibbon(Object o, double width);
/**
\index{DXRibbon}
Produces a ribbon of the given width from a path or group of paths.
The ribbon is perpendicular to the normal and parallel to the tangent
at all points on the path, where the normals and tangents are provided
by ``normals'' and ``tangents'' components if present or approximated
from the path otherwise.  The normals (given or computed) are
translated to the generated vertices and associated with the ribbon
for shading.  Returns the ribbon, or returns null and sets the error
code to indicate an error.
**/

Object DXTube(Object o, double diameter, int n);
/**
\index{DXTube}
Produces a tube of the given diameter from a path or group of paths.
The cross section of the tube an {\tt n}-gon in a plane parallel to
the normal and perpendicular to the tangent at all points on the path,
where the normals and tangents are provided by ``normals'' and
``tangents'' components if present or approximated from the path
otherwise.  Normals to the tube are computed and associated with the
tube for shading.  Returns the tube, or returns null and sets the
error code to indicate an error.
**/

/*
\section{Glyphs}
Glyphs are small geometric objects such as vectors glyphs that can be
used to convey local information.\marginpar{More to come.}
*/

Object DXVectorGlyph(Point p, Vector v, double d, RGBColor c);
/**
\index{DXVectorGlyph}
Creates a vector glyph at {\tt p} with direction and length specified by
{\tt v}.  The diameter is specified by {\tt d} and the color is specified
by {\tt c}.  Returns an object or null to indicate an error.
**/

#endif /* _DXI_GEOMETRY_H_ */

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif
