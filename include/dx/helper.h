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

#ifndef _DXI_HELPER_H_
#define _DXI_HELPER_H_

/* TeX starts here. Do not remove this comment. */

/*
\section{Field construction}

This section describes routines that aid in the construction of fields
meeting the expectations of the rest of the system with regard to
component names and attributes.

\paragraph{Points and pointwise data.}

Associated with a field may be a number of pointwise components: ``points'',
``data'', ``colors'', ``opacities'', surface ``normals'', ``texture''
map indices.  These components are all expected to be the same size; this is
indicated by their each having a ``dep'' attribute of ``points''.  The
following routines aid in constructing such components.
*/

Field DXAddPoint(Field f, int id, Point p);
Field DXAddColor(Field f, int id, RGBColor c);
Field DXAddFrontColor(Field f, int id, RGBColor c);
Field DXAddBackColor(Field f, int id, RGBColor c);
Field DXAddOpacity(Field f, int id, double o);
Field DXAddNormal(Field f, int id, Vector v);
Field DXAddFaceNormal(Field f, int id, Vector v);
/**
\index{DXAddPoint}\index{DXAddColor}\index{DXAddOpacity}\index{DXAddNormal}
\index{DXAddFaceNormal}\index{DXAddFrontColor}\index{DXAddBackColor}
Adds one point, color, opacity, or normal to {\tt f} with the
specified {\tt id}.  Creates an appropriate component if necessary.
(In the future we may add e.g. {\tt AddPoint4d()} for four-component
double-precision points).  Return {\tt f}, or return null and set the
error code to indicate an error.

Note: these routines are suitable for adding a small number of points,
or for rapid prototyping.  For better performance, see {\tt
DXAddArrayData()} for a discussion of direct access routines to arrays.
**/

Field DXAddPoints(Field f, int start, int n, Point *p);
Field DXAddColors(Field f, int start, int n, RGBColor *c);
Field DXAddFrontColors(Field f, int start, int n, RGBColor *c);
Field DXAddBackColors(Field f, int start, int n, RGBColor *c);
Field DXAddOpacities(Field f, int start, int n, float *o);
Field DXAddNormals(Field f, int start, int n, Vector *v);
Field DXAddFaceNormals(Field f, int start, int n, Vector *v);
/**
\index{DXAddPoints}\index{DXAddColors}\index{DXAddOpacities}\index{DXAddNormals}
\index{DXAddFaceNormals}\index{DXAddFrontColors}\index{DXAddBackColors}
Adds {\tt n} points, user data, colors, opacities, or normals with ids
starting with {\tt start} to {\tt f}.  Creates an appropriate
component if necessary.  Return {\tt f}, or return null and set the
error code to indicate an error.

Note: these routines are suitable for adding a small number of points,
or for rapid prototyping.  For better performance, see {\tt
DXAddArrayData()} for a discussion of direct access routines to arrays.
**/

/*
\paragraph{Connections.}
This section describes routines for describing the interpolation
elements of a field.  These routines result in the creation of a
``connections'' component with an appropriate ``element type''
attribute.
*/

Field DXAddLine(Field f, int id, Line l);
Field DXAddTriangle(Field f, int id, Triangle t);
Field DXAddQuad(Field f, int id, Quadrilateral q);
Field DXAddTetrahedron(Field f, int id, Tetrahedron t);
/**
\index{DXAddLine}\index{DXAddTriangle}\index{DXAddTetrahedron}\index{DXAddQuad}
Adds one triangle or tetrahedron to {\tt f} with the specified {\tt
id}.  Creates an appropriate component if necessary.  Return {\tt f},
or return null and set the error code to indicate an error.
**/

Field DXAddLines(Field f, int start, int n, Line *l);
Field DXAddTriangles(Field f, int start, int n, Triangle *t);
Field DXAddQuads(Field f, int start, int n, Quadrilateral *q);
Field DXAddTetrahedra(Field f, int start, int n, Tetrahedron *t);
/**
\index{DXAddLines}\index{DXAddTriangles}\index{DXAddTetrahedra}
Adds {\tt n} triangles or tetrahedra to {\tt f} with identifiers
starting with {\tt start} to {\tt f}.  Creates an appropriate
component if necessary.  Return {\tt f}, or return null and set the
error code to indicate an error.
**/

Field DXSetConnections(Field f, char *type, Array a);
/**
\index{DXSetConnections}
Sets the ``connections'' component of {\tt f} to {\tt a} and gives it
an ``element type'' attribute of {\tt type}.  Returns {\tt f}, or
returns null and sets the error code to indicate an error.
**/

Array DXGetConnections(Field f, char *type);
/**
\index{DXGetConnections}
Gets the ``connections'' component of {\tt f} and checks to see if it
has an ``element type'' attribute of {\tt type}.  Returns the
connections array, or returns null but does not set the error code if
no ``connections'' component was present or if the ``element type''
attribute did not match.
**/

/*
\paragraph{Standard components.}
This section describes routines for creating and manipulating standard
components of a field.
*/

Field DXEndField(Field f);
/**
\index{DXEndField}
This routine 1) creates all the standard components that are expected
by various processing modules, such as ``box'', ``neighbors'', etc.,
and 2) sets default values for ``ref'' and ``dep'' attributes.  This
{\em must} be called after creating any field.  Returns {\tt f}, or
returns null and sets the error code to indicate an error.
**/

int DXEmptyField(Field f);
/**
\index{DXEmptyField}
Returns 1 if the field {\tt f} is an empty field, 0 otherwise.  An
empty field is one with no components, no ``positions'' component, or
a ``positions'' component with zero positions.  Modules may use this
to ignore empty fields, so that the absence of required components in
an empty field is not treated as an error.
**/

Field DXChangedComponentValues(Field f, char *component);
Field DXChangedComponentStructure(Field f, char *component);
/**
\index{DXChangedComponentValues}\index{DXChangedComponentStructure}
{\tt DXChangedComponentValues()} deletes all components of {\tt f} that
have a ``der'' attribute naming the specified component.  This is
typically used when you have changed the values of the items of an
array, such as the values of the ``data'' component), but you have not
changed the number of items.

{\tt DXChangedComponentStructure()} deletes all components of {\tt f}
that have a ``dep'', ``der'', or ``ref'' attribute naming the
specified component.  This is typically used when you have changed the
number of items in an array, such as the number of items in the
``positions'' component.  Both of these routines recursively apply
{\tt DXChangedComponentStructure()} to all components they delete.
Return {\tt f}, or return null and set the error code to indicate an
error.
**/

Object DXValidPositionsBoundingBox(Object o, Point *box);
Object DXBoundingBox(Object o, Point *box);
/**
\index{DXBoundingBox}
Computes the bounding box of a group or field {\tt o}.  Adds to {\tt
o} or any of its descendendants that are fields a ``box'' component
consisting of an array of $2^d$ points which are the corners of the
bounding box, where $d$ is the dimensionality of the data.  For data
of dimensionality three or less, returns in the array pointed to by
{\tt box} the eight corner points; for dimensionalities less than
three, the extra dimensions are treated as zero in the box returned by
the {\tt DXBoundingBox()} routine.  Returns {\tt o}, or returns null but
does not set the error code if it is not possible to define a bounding
box for the given input (for example, the number of positions was 0).
**/

Array DXNeighbors(Field f);
/**
\index{DXNeighbors}
Returns the neigbors array of field {\tt f}, as described in the
definition of the ``neighbors'' component in Chapter ??? of the User
Manual.  If the ``neighbors'' component already exists, it is simply
returned.  If it does not exist, a ``neighbors'' component is
computed.  Returns the ``neighbors'' array, or returns null and sets
the error code to indicate an error.
**/

Error DXStatistics(Object o, char *component, float *min, float *max, 
		 float *avg,  float *sigma);
/**
\index{DXStatistics}
Returns combined statistical information for an object.  If {\tt min}
is not null, this routine returns the minimum value in {\tt *min}.  If
{\tt max} is not null, this routine returns the maximum value in {\tt
*max}.  If {\tt avg} is not null, this routine returns the average
value in {\tt *avg}.  if {\tt sigma} is not null, this routine returns
the standard deviation in {\tt *sigma}.  The ``statistics'' component
is added to fields for which it does not already exist.  If this
routine is called with an array for {\tt o}, it ignores {\tt
component} and returns statistics for the array.  Returns {\tt OK}, or
returns {\tt ERROR} and sets the error code to indicate an error.
**/

Array DXScalarConvert(Array a);
/**
\index{DXScalarConvert}
Creates and returns a new array with the contents of array {\tt a}
converted to scalar values using the same conversion as the DXStatistics
routine uses for vector or matrix data.
**/

Error DXColorNameToRGB(char *name, RGBColor *color);
/**
\index{DXColorNameToRGB}
Returns the corresponding r,g,b value for the color "name" if name is
a valid color.  Uses DXCOLORS as the names file, or /usr/lpp/dx/lib/colors.txt
if DXCOLORS is not set.
**/

Object DXApplyTransform(Object o, Matrix *m);
/**
\index{DXApplyTransform}
Traverses an object, concatenating matricies as encountered, and 
applys the results to the geometric data found in the fields.
The result is a copy of the input with all matricies removed.
The concatenated matrices are applied to arrays found in fields
named "positions", "normals", "gradient", "binormals" and "tangents".
In the case of positions, the concatenated matrices are directly
applied; in the remaining cases (which comprise vector data), the
adjoint transopose of the concatenated matrix is applied.  Additionally,
arrays which carry a "geometric" attribute are also transformed; those
for which the attribute's value is "positional" are transformed by the
matrix, while those for which the attribute's value is "vector" are
transformed by the adjoint transpose.
**/

#endif /* _DXI_HELPER_H_ */

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif
