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

#ifndef _DXI_RESAMPLING_H_
#define _DXI_RESAMPLING_H_

/* TeX starts here. Do not remove this comment. */


/*
\section{Interpolation and Mapping}

The interpolation service described below provides a means of making
queries about values of a function $y(x)$ defined in terms of a set of
points $x_i$, values $y_i$, and basis functions $b_i(x)$.  The
interpolation service centers around an {\tt Interpolator} object.  An
interpolator object is similar to an open file.  To calculate
interpolated values, first create an interpolator object, which
constructs a data structure that allows efficient access for
interpolating values from a given field or composite field.  The $x_i$
are specified by the ``points'' component of the given field(s), the
$y_i$ are specified by the ``data'' component, and the $b_i(x)$ are
defined implicitly by the element type.

The interpolator object provides a consistent interface to calling
applications while providing interpolation methods that are specific
to the representation used in the field data object.  The choice of
interpolation method is based on factors such as the hierarchical
structure of the data model, whether compact data formats are used,
the primitive types used, and the interpolation model.

Interpolator objects create internal data structures that are used to
speed interpolation.  These structures are created during the
initialization of the interpolator.  Initialization can occur in two
ways: either immediately during the creation of the interpolator, or
on demand during the interpolation process.  Delayed initialization is
of particular use when the data being interpolated is partitioned, and
samples may not be interpolated from every partition, thereby avoiding
the potentially costly initialization of partitions of the
interpolated data that are never required.  However, if the
interpolator is intended for sharing, it must be fully initialized
prior to copying.  This can be done in parallel prior to the creation
of sub-tasks.

Interpolator objects utilize information gathered in prior
interpolations to speed subsequent interpolations.  For this reason,
interpolator objects contain data that is specific to the process
using the interpolator.  This prevents interpolators from being shared
among parallel tasks.  Instead, each parallel process must have its
own interpolator.  This can be provided efficiently by creating a
single, fully initialized interpolator which is passed to the parallel
subtasks that wish to use it; the subtasks then {\it copy} this
interpolator and use the copy for local interpolation.  When this
approach is used, the parent interpolator cannot utilize delayed
initialization; instead, the parent interpolator must be fully
initialized prior to copying.

Data objects interpolated through this interface must be of the same
dimensionality as the space in which they are embedded: thus we can
interpolate triangles embedded in the plane, but not in 3-space.
*/

enum interp_init {
    INTERP_INIT_DELAY,
    INTERP_INIT_IMMEDIATE,
    INTERP_INIT_PARALLEL
};

Interpolator DXNewInterpolator(Object o, enum interp_init init, float fuzz);
/**
\index{DXNewInterpolator}
Creates an {\tt Interpolator} object for interpolating {\tt o}.
The initialization type is specified by setting the {\tt init}
argument to {\tt INTERP\_INIT\_DELAY}, {\tt INTERP\_INIT\_IMMEDIATE}, or 
{\tt INTERP\_INIT\_PARALLEL}.

The {\tt fuzz} value assigns a fuzz factor to the interpolation process:
any sample falling within this distance of a valid primitive of the 
object {\tt o} is assumed to be inside that primitive.  When this point
lies geometrically outside the primitive, an appropriate result is 
extrapolated.  Any positive or zero value is used as the fuzz factor;
a negative value indicates that the interpolator should determine its own
fuzz factor.

Returns the interpolator, or returns null and sets the error code to
indicate an error.
**/

Interpolator DXInterpolate(Interpolator interpolator, int *n,
			 float *points, Pointer result);
/**
\index{DXInterpolate}
Interpolates up to {\tt *n} points in the data object associated with
{\tt interpolator}.  The {\tt points} parameter is a pointer to a list
of sample points to be interpolated.  The {\tt result} is a pointer to
a buffer large enough to hold {\tt *n} elements of the type of the
data object associated with {\tt interpolator}.  The input sample
points are interpolated sequentially until a point lying outside the
data model is encountered, at which time interpolation terminates.
This routine returns in {\tt *n} returns the number of points that
remained to be interpolated when a point outside the data model is
found; this is not considered to be an error.

The list of sample points consists of {\tt *n} points of the same
dimensionality as the object being interpolated: thus (x) points
are used to interpolate along the line, (x,y) points are used to
interpolate in the plane and (x,y,z) points in 3-space.

This routine returns {\tt interpolator} on success, or returns null
and sets the error code to indicate on error.
**/

Interpolator DXLocalizeInterpolator(Interpolator interp);
/**
\index{DXLocalizeInterpolator}
Interpolators require access to various data structures that, by default,
exist in shared memory.  If the data being interpolated is relatively
small and repeatedly accessed, the contents of these structures may be
copied into local memory for faster access.  Returns the localized
interpolator, or returns null and sets the error code to indicate an
error.
**/

Object DXMap(Object object, Object map, char *src, char *dst);
/**
\index{DXMap}
Interpolates samples selected by {\tt object} from {\tt map}.  This
procedure provides a simple generic tool for interpolation.  The {\tt object}
may be either a field, a composite field, or an array.  In the first
two cases, the component specified by {\tt src} is used to 
sample {\tt map}; the results of the interpolation is placed in the
component specified by {\tt dst}, and {\tt object} is returned.
If {\tt object} is an array, it is used directly to interpolate {\tt map},
and an array containing the interpolated values is returned;  in this
case, the {\tt src} and {\tt dst} parameters are ignored.

If {\tt map} is an array, this routine creates a resulting array that
consists of the appropriate number of copies of the contents of the
{\tt map} array (which must contain exactly one item).  This result is
then handled as above: if {\tt object} is a field or a composite
field, the result array is added to the field using the component name
specified by {\tt dst}.  Otherwise, the resulting array is returned.

Alternately, {\tt map} represents a data field to be interpolated.  In
this case it must be either a field, a composite field or an interpolator.
If it is a field or composite field, an interpolator will be created.

Returns an object as described above, or returns null and sets the
error code to indicate an error.
**/

Array DXMapArray(Array index, Interpolator map, Array *invalid);
/**
\index{DXMapArray}
Provides a lower-level mapping function.  {\tt index} is an array
containing points to be sampled from the interpolator {\tt map}.  The
result is returned as an array.  If {\tt invalid} is not null, an
array that indicates invalid data in which uninterpolated elements are
tagged {\tt DATA\_INVALID} is returned.  If an invalid-data array
corresponding to {\tt index} exists prior to the call to {\tt
DXMapArray}, it should be passed in through {\tt *invalid}.  Returns
{\tt index}, or returns null and sets the error code to indicate an
error.
**/

Object DXMapCheck(Object input, Object map, char *index, 
		Type *type, Category *category, int *rank, int *shape);
/**
\index{DXMapCheck}
Verifies that the types of {\tt input} and {\tt map} are valid.  If the
map is an array, it must contain a single element.  If the map is not
an array, implying the need for interpolation from the map, the type,
category, rank and shape of the input component specified by {\tt
index} must match that of the positions component of the map.  The
type, category, rank and shape of the map (and of the data object
produced by this mapping) are returned in the corresponding arguments.
Returns the {\tt input} argument if {\tt input} and {\tt map} are
valid for mapping, otherwise returns null but does not set the error
code.\marginpar{Is this true?}
**/

#endif /* _DXI_RESAMPLING_H_ */

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif
