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

#ifndef _DXI_BASIC_H_
#define _DXI_BASIC_H_

/* TeX starts here. Do not remove this comment. */

/*
\section{Basic types}

This section describes some basic data types used by the system including
points, vectors, triangles, colors and matrices.

\paragraph{Points and vectors.}
Points are represented by the {\tt Point} structure.  For programmer
convenience, Data Explorer provides a routine that constructs a point
structure.  Vectors are represented by the same structure as points,
but are defined as a separate type for preciseness in interface
definitions.
*/

typedef struct point {
    float x, y, z;
} Point, Vector;

typedef int PointId;

Point DXPt(double x, double y, double z);
Point DXVec(double x, double y, double z);
/**
\index{Point}\index{Vector}\index{DXPt}\index{DXVec}
Constructs a point or a vector with the given coordinates.
**/

/*
\paragraph{Lines, triangles, quadrilaterals, and tetrahedra.}
These data structures define the interpolation elements of an
object.  They refer to points by point identifiers.
\index{Line}\index{Triangle}\index{Tetrahedra}
*/

typedef struct line {
    PointId p, q;
} Line;

typedef struct triangle {
    PointId p, q, r;
} Triangle;

typedef struct quadrilateral {
    PointId p, q, r, s;
} Quadrilateral;

typedef struct tetrahedron {
    PointId p, q, r, s;
} Tetrahedron;

typedef struct cube {
    PointId p, q, r, s, t, u, v, w;
} Cube;

Line DXLn(PointId p, PointId q);
Triangle DXTri(PointId p, PointId q, PointId r);
Quadrilateral DXQuad(PointId p, PointId q, PointId r, PointId s);
Tetrahedron DXTetra(PointId p, PointId q, PointId r, PointId s);
/**
\index{DXLn}\index{DXTri}\index{DXQuad}\index{DXTetra}
Constructs a line, triangle, quadrilateral, or tetrahedron given the
appropriate number of point identifiers.
**/

/*
\paragraph{Colors.}
Colors define the photometry of an object; they may be associated with
points, triangles, or lights, and may be used to define the
reflectance or the opacity of an object.  Colors are floating point
numbers and so are open ended, as is light intensity in the real
world; however, real output devices have a limited light intensity
range.  In general, the range of intensities from 0.0 to 1.0 is by
default mapped onto the displayable intensities on the output device,
so colors should normally be specified in this range.  For programmer
convenience, Data Explorer provides a routine that constructs an DXRGB
color structure.
*/

typedef struct rgbcolor {
    float r, g, b;
} RGBColor;

typedef struct rgbbytecolor {
    ubyte r, g, b;
} RGBByteColor;

RGBColor DXRGB(double r, double g, double b);
/**
\index{RGBColor}\index{DXRGB}
Constructs an DXRGB color structure with the given components.
**/

/*
\paragraph{Angles.}
Angles are expressed as floating point numbers in radians.  The following
macros help to express angles in other units that might be more convenient.
For example, {\tt 5*DEG} is 5 degrees in radians.
\index{Angle}
*/

typedef double Angle;
#define DEG (6.283185307179586476925287/360)
#define RAD (1)

/*
\paragraph{Transformation matrices.}
\label{matrixsec}
Transformation matrices specify the relationship between objects. For
example, when a renderable object is included in a model, a
transformation matrix specifies the placement of the object in the
model.  In the Data Explorer, a transform is a $4\times3$ matrices
specifying rotation and translation.  This is a homogeneous matrix
less the part that computes the $w$ component of the result.  The $w$
component is used for perspective, which is specified by a camera and
is not needed here.  \index{Matrix}\index{Identity}
*/

typedef struct matrix {
    /* xA + b */
    float A[3][3];
    float b[3];
} Matrix;

/*
Transformation matrices may be specified in a number of ways:
*/

Matrix DXRotateX(Angle angle);
Matrix DXRotateY(Angle angle);
Matrix DXRotateZ(Angle angle);
/**
\index{DXRotateX}\index{DXRotateY}\index{DXRotateZ}
Returns a matrix that specifies a rotation about the $x$, $y$
or $z$ axis.
**/

Matrix DXScale(double x, double y, double z);
/**
\index{DXScale}
Returns a matrix that specifies a scaling along the $x$, $y$ and/or
$z$ axes.
**/

Matrix DXTranslate(Vector v);
/**
\index{DXTranslate}
Returns a matrix that specifies a translation by {\tt v}.
**/

Matrix DXMat(
    double a, double b, double c,
    double d, double e, double f,
    double g, double h, double i,
    double j, double k, double l
);
/**
\index{DXMat}
Returns a matrix with the specified components.
**/

/*
\paragraph{Basic operations.}
A number of basic operations on the {\tt Matrix}, {\tt Point}, and
{\tt Vector} types are provided.  The operations defined below are
declared as operating on type {\tt Vector}, but they work on {\tt Point} as
well, because {\tt Point} and {\tt Vector} are both {\tt typedef}s for the same
structure.  These operations all take their arguments by value and return
the result.
*/

Vector DXNeg(Vector v);
Vector DXNormalize(Vector v);
double DXLength(Vector v);
/**
\index{DXNeg}\index{DXNormalize}\index{DXLength}
Performs unary vector operations of negation, normalization and length.
**/

Vector DXAdd(Vector v, Vector w);
Vector DXSub(Vector v, Vector w);
Vector DXMin(Vector v, Vector w);
Vector DXMax(Vector v, Vector w);
/**
\index{DXAdd}\index{DXSub}\index{DXMin}\index{DXMax}
Performs binary vector operations of addition, subtraction, min and max.
**/

Vector DXMul(Vector v, double f);
Vector DXDiv(Vector v, double f);
/**
\index{DXMul}\index{DXDiv}
Multiplies or divides a vector by a float.
**/

float DXDot(Vector v, Vector w);
Vector DXCross(Vector v, Vector w);
/**
\index{DXDot}\index{DXCross}
Forms the dot product or cross product of two vectors.
**/

Matrix DXConcatenate(Matrix s, Matrix t);
/**
\index{DXConcatenate}
Returns a matrix that is equivalent to transformation matrix {\tt s}
followed by transformation matrix {\tt t}.
**/

Matrix DXInvert(Matrix t);
Matrix DXTranspose(Matrix t);
Matrix DXAdjointTranspose(Matrix t);
float DXDeterminant(Matrix t);
/**
\index{DXInvert}\index{DXTranspose}\index{Adjoint}\index{DXDeterminant}
Computes the matrix inverse, transpose, and adjoint transpose, and
determinant, respectively.
**/

Vector DXApply(Vector v, Matrix t);
/**
\index{DXApply}
Forms the product of vector {\tt v} (interpreted as a row vector) with
the matrix~{\tt t}.
**/

#endif /* _DXI_BASIC_H_ */

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif
