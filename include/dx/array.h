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

#ifndef _DXI_ARRAY_H_
#define _DXI_ARRAY_H_

/* TeX starts here. Do not remove this comment. */

/*
\section{Array class}
\label{arraysec}
Array objects store the actual user data, positions, connections, or
photometry information.  Array objects may be used to manage large
dynamic arrays of data.  Arrays may store arbitrary, irregular
information, or information that corresponds to regular grids.  This
section first describes the generic operations that are applicable to
all arrays, then operations specific to irregular arrays, and finally
operations specific to compact arrays.

Each array {\tt a} contains some number of items, numbered from {\tt
0} to {\tt n-1}, where {\tt n} is the number of items in {\tt a}.
Each item consists of a fixed number of elements all of the same type,
specified when the array is created.  The constants used to specify
the types are defined in section \ref{objectsec}.
*/

Array DXNewArrayV(Type t, Category c, int rank, int *shape);
Array DXNewArray(Type t, Category c, int rank, ...);
/**
\index{DXNewArrayV}\index{DXNewArray}
Creates an irregular {\tt Array} object.  Each item is a scalar,
vector, matrix, or tensor whose rank is specified by {\tt rank}
(number of dimensions) and whose shape is specified either by the
array {\tt shape} (for {\tt DXNewArrayV()}) or by the last {\tt rank}
arguments (for {\tt DXNewArray()}).  Each entry in the item is a real,
complex, or quaternion according to {\tt c}, with coefficients that
are integer, single precision, double precision, etc.  according to
{\tt t}.  The constants used to specify {\tt t} and {\tt c} are
defined in section \ref{objectsec}.  The {\tt Array} initially
contains no items.  Returns the array, or returns null and sets the
error code to indicate an error.
**/

Class DXGetArrayClass(Array a);
/**
\index{DXGetArrayClass}
Returns the subclass of an array object.  This return value will be
{\tt CLASS\_ARRAY} if the object is an irregular array.  If it is a
compact array the return value will be one of {\tt
CLASS\_REGULARARRAY}, {\tt CLASS\_PRODUCTARRAY}, {\tt
CLASS\_PATHARRAY}, or {\tt CLASS\_MESHARRAY}.
**/

Array DXGetArrayInfo(Array a, int *items, Type *type, Category *category,
		   int *rank, int *shape);
/**
\index{DXGetArrayInfo}
If {\tt items} is not null, this routine returns in {\tt *items} the
number of items currently in the array.  If {\tt type} is not null, it
returns in {\tt *type} the type of each item.  If {\tt category} is
not null, it returns in {\tt *category} the category of each item.  If
{\tt rank} is not null, it returns in {\tt *rank} the number of
dimensions in each item.  If {\tt shape} is not null, it returns in
{\tt *shape} an array of the extents of each dimension of the items.
Returns {\tt a}, or returns null and sets the error code to indicate
an error.
**/

Array DXTypeCheckV(Array a, Type type, Category category, int rank, int *shape);
Array DXTypeCheck(Array a, Type type, Category category, int rank, ...);
/**
\index{DXTypeCheckV}\index{DXTypeCheck}
Returns {\tt a} if the type, category, rank, and shape of {\tt a} are
as specified, or null if not.  The shape is specified by the array
{\tt shape} for {\tt DXTypeCheckV()} or by the last {\tt rank} arguments
for {\tt DXTypeCheck()}.  For {DXTypeCheckV()}, if {\tt shape} is null the
type, category and rank are checked, but the shape is not checked.
Returns {\tt a}, or returns null and sets the error code to indicate
an error (if for example the type does not match).
**/

Pointer DXGetArrayData(Array a);
/**
\index{DXGetArrayData}
Returns a pointer to a C array in global memory of items constituting
the data stored in {\tt a}.  For irregular arrays, the pointer points
to the actual data that was stored with array; this data may be
changed directly to change the contents of the array.  For compact
arrays (regular, grid, path, or mesh arrays, described below), this
routine expands the compact data and returns a pointer to the result;
such data should not be changed.  The returned array contains {\tt n}
entries numbered from {\tt 0} to {\tt n-1}, where {\tt n} is the
number of items in {\tt a}.  The values of items in an irregular array
that have not been set by {\tt DXAddArrayData()} are undefined.  Returns
a pointer to the data, or returns null and sets the error code to
indicate an error.
**/

int DXGetItemSize(Array a);
/**
\index{DXGetItemSize}
Returns the size in bytes of each individual item of {\tt a}, or returns
0 to indicate an error.
**/

Pointer DXGetArrayDataLocal(Array a);
/**
\index{DXGetArrayDataLocal}
Returns a pointer to a C array of items constituting a local copy of
the data stored in {\tt a}, and records in {\tt a} the fact that a
local copy of {\tt a} exists on the processor on which the call is
made.  The returned array contains {\tt n} entries numbered from {\tt
0} to {\tt n-1}, where {\tt n} is the number of items in {\tt a}.  The
values of items in the array that have not been set by {\tt
DXAddArrayData()} are undefined.  You must release the local storage for
the data when you no longer need it by calling {\tt
DXFreeArrayDataLocal()}.  On uniprocessor architectures, this routine
just returns a pointer to the array data.  In either case, the data
must not be modified directly.  Returns a pointer to the data, or
returns null and sets the error code to indicate an error.
**/

Array DXFreeArrayDataLocal(Array a, Pointer data);
/**
\index{DXFreeArrayDataLocal}
Indicates that a reference to the local copy {\tt data} of the data for
array {\tt a} no longer exists.  If {\tt data} is actually a pointer
to the global data, this call is ignored, so that it is always safe to
call this routine on the result of {\tt DXGetArrayDataLocal()}, whether
or not {\tt data} really is a local pointer.  When the last local
reference on a given processor is released, the local storage for the
array data will be freed.  Returns {\tt a}, or returns null and sets
the error code to indicate an error.
**/

/*
\paragraph{Irregular arrays.}
Irregular arrays are used to store data that exhibit no particular
regularity.  They may be used to manage dynamically growing
collections of data whose size is not known in advance.  Irregular
arrays are created with {\tt DXNewArray()}, as documented above, and
they start out containing no items; data is added to an irregular
array by calling {\tt DXAddArrayData}.  Data is retrieved from the array
by calling {\tt DXGetArrayData}, as documented above.  The routines
described in the preceding section apply to both irregular and compact
arrays.  The routines described in this section apply only to
irregular arrays.
*/

Array DXAddArrayData(Array a, int start, int n, Pointer data);
/**
\index{DXAddArrayData}
Adds {\tt n} more items to {\tt a}, numbered starting at {\tt start}.
These may replace or supplement items already defined. If {\tt data}
is not null, this routine copies the data into the array; otherwise,
it increases the number of items in the array, but leaves them
uninitialized.  {\tt DXAddArrayData} may allocate more storage than is
needed; use {\tt DXAllocateArray()} to pre-allocate exactly the amount
of storage you need, or {\tt DXTrim()} to reduce the allocation to the
number of items actually in the array.  Returns {\tt a}, or returns
null and sets the error code ato indicate an error.
**/

Array DXAllocateArray(Array a, int n);
/**
\index{DXAllocateArray}
This routine allocates room for at least {\tt n} items in array {\tt
a}.  If you know in advance how many items you will add to an array,
you can make the creation of the array more efficient by allocating
the storage in advance.  This does not change the number of items in
{\tt a}; only {\tt DXAddArrayData()} changes the number of items.
Returns {\tt a}, or returns null and sets the error code to indicate
an error.
**/

Array DXTrim(Array a);
/**
\index{DXTrim}
Under some circumstances, more space than is necessary to hold the
items added to {\tt a} may have been allocated.  This can happen if
you have called {\tt DXAllocateArray()}; it can also happen when you
call {\tt DXAddArrayData()}.  This extra space can be freed by calling
{\tt DXTrim()}.  The {\tt DXEndField()} routine automatically calls {\tt
DXTrim()} on all components of a field.  Returns {\tt a}, or returns
null and sets the error code to indicate an error.
**/

/*
\paragraph{Usage notes.}
There are four ways to use irregular arrays: (1) add the items one at
a time using {\tt DXAddArrayData(a, i, 1, \&item)}; (2) add the items in
batches using {\tt DXAddArrayData(a, i, n, items)}; (3) add the items
all at once using {\tt DXAddArrayData(a, 0, n, items)}; or (4) allocate
the storage by calling {\tt DXAddArrayData(a, 0, n, NULL)}, get a
pointer to the storage in global memory with {\tt DXGetArrayData(a)},
and put the items directly into global memory ``by hand.''
*/

/*
The data structures used to implement arrays are not designed for sparse
array storage; it is not advisable to use {\tt DXAddArrayData()} to leave large
holes in the array.
*/

/*
\paragraph{Compact arrays.}
{\em Compact arrays} allow compact encodings of regularity of
positions and regularity of connections.  Four subclasses of arrays
encode respectively one-dimensional and multi-dimensional regular
positions and connections:
\begin{center}
\begin{tabular}{l|ll}
		& positions & connections \\
\hline
one-dimensional	& {\tt RegularArray} & {\tt PathArray} \\
$n$-dimensional	& {\tt ProductArray} & {\tt MeshArray} \\
\end{tabular}
\end{center}
Most of the generic array operations are available for these
subclasses.  In particular applying {\tt DXGetArrayData()} to a compact
array will expand out the data; this is the preferred technique of
explicitly expanding a compact array.  Of course, it is preferrable to
code your algorithm if possible so that no explicit expansion of the
array is performed.

In addition to the low-level operations described in a later section
for creating the various compact array encodings, the Data Explorer
library provides the following higher-level operations for creating
the common case of a regular grid of positions or connections.  In
general it is preferable to use these routines where possible, because
more Data Explorer functions support these cases efficiently.
*/

Array DXTrimItems(Array a, int nitems);
/**
\index{DXTrimItems}
It may desirable to remove elements at the end of an array to shrink 
the array without having to copy it. {\tt DXTrimItems()} will make the
array nitems long removing items from the end of the array and 
automatically calling {\tt DXTrim()} to free the extra array memory.
**/


Array DXMakeGridPositionsV(int n, int *counts, float *origin, float *deltas);
Array DXMakeGridPositions(int n, ...);
/**
\index{DXMakeGridPositionsV}\index{DXMakeGridPositions}
Constructs an {\tt n}-dimensional regular grid.  A regular grid is
specified by an {\tt n}-dimensional {\tt origin}, a set of {\tt n}
{\tt n}-dimensional {\tt deltas}, and a set of {\tt n} {\tt counts}.
The grid consists of the lattice of points defined by the origin plus
integer multiples of each of the delta vectors up to the count
specified for each vector.  For {\tt DXMakeGridPositionsV()}, the number
of points in the direction of each of the {\tt n} delta vectors is
given by {\tt counts} array (which contains {\tt n} integers), the
origin is given by the {\tt n}-dimensional vector pointed to by {\tt
origin}, and the deltas are given by {\tt n*n} numbers interpreted as
{\tt n} {\tt n}-dimensional vectors pointed to by {\tt deltas}.  For
{\tt DXMakeGridPositions()}, the counts, origin, and deltas respectively
are given as the last {\tt n + n*n + n} arguments.  The resulting
array is the product of {\tt n} regular arrays, constructed using the
compact array objects described below; this routine is included to
simplify the process of creating the common case of a regular grid.
Returns the array, or returns null and sets the error code to indicate
an error.
**/

Array DXQueryGridPositions(Array a, int *n, int *counts,
		       float *origin, float *deltas);
/**
\index{DXQueryGridPositions}
This routine returns null if {\tt a} is not a regular grid of the sort
constructed by {\tt DXMakeGridPositions()}.  If {\tt n} is not null, it
returns the number of dimensions in the grid in {\tt *n}.  If {\tt
counts} is not null, it returns the number of points along each delta
vector in the array pointed to by {\tt counts}.  If {\tt origin} is
not null, it returns the {\tt n}-dimensional origin in the array
pointed to by {\tt origin}.  If {\tt deltas} is not null, it returns
the {\tt n} {\tt n}-dimensional delta vectors in the array pointed to
by {\tt deltas}.  Returns {\tt a} if it is a regular grid, or returns
null but does not set the error code if it is not.
**/

Array DXMakeGridConnectionsV(int n, int *counts);
Array DXMakeGridConnections(int n, ...);
/**
\index{DXMakeGridConnectionsV}\index{DXMakeGridConnections} Constructs an
array of {\tt n}-dimensional regular grid connections, that is a set
of {\tt n}-dimensional cubes (hypercubes).  For {\tt
DXMakeGridConnectionsV()}, the number of points along each axis is given
by {\tt counts}.  For {\tt DXMakeGridConnections()}, the counts are
given as the last {\tt n} arguments.  The resulting array is the
product of {\tt n} regular connections arrays, constructed using the
routines described below; this routine is included to simplify the
process of creating the common case of regular grid connections.
Returns the array, or returns null and sets the error code to indicate
an error.
**/

Array DXQueryGridConnections(Array a, int *n, int *counts);
/**
\index{DXQueryGridConnections}
Returns null if {\tt a} is not an array of regular grid connections of
the sort constructed by {\tt DXMakeGridConnections()}.  If {\tt n} is
not null, returns the number of dimensions in the grid in {\tt *n}.
If {\tt counts} is not null, returns the number of points along each
axis in the array pointed to by {\tt counts}.  Returns {\tt a} if it
is a grid connections array, or returns null but does not set the error
code if it is not.
**/

/*
\paragraph{Regular arrays.}
{\em Regular arrays} encode linear positional regularity.  A regular
array is a set of $n$ points lying on a line with a constant spacing
between them, representing one-dimensional regular positions.  (The
points themselves may be in a higher-dimensional space, but they must
lie on a line.)  All regular arrays must have category real (as
opposed to complex or quaternion), type float or double, and rank 1;
the shape is then the dimensionality of the space in which the points
are imbedded.
*/

RegularArray DXNewRegularArray(Type t, int dim, int n,
			     Pointer origin, Pointer delta);
/**
\index{DXNewRegularArray}
Creates a new {\tt Array} object representing a regular array of {\tt
n} points starting at {\tt origin} with a spacing of {\tt delta}.
The type is specified by {\tt t}, and must be {\tt TYPE\_FLOAT} or {\tt
TYPE\_DOUBLE}.  The rank is assumed to be 1, and the shape is {\tt
dim}.  Both {\tt origin} and {\tt delta} are assumed to point to items
of the same type as the items in {\tt a}.  Returns the regular array,
or returns null and sets the error code to indicate an error.
**/

RegularArray DXGetRegularArrayInfo(RegularArray a, int *count,
				 Pointer origin, Pointer delta);
/**
\index{DXGetRegularArrayInfo}
If {\tt count} is not null, this routine returns in {\tt *count} the
number of points. If {\tt origin} is not null, it returns in {\tt
*origin} the position of the first point. If {\tt delta} is not null,
it returns in {\tt *delta} the spacing between the points. Both {\tt
origin} and {\tt delta} must point to buffers large enough to hold one
item of the type of {\tt a}.  the information about {\tt a} may be
obtained by calling {\tt DXGetArrayInfo()}.  Returns {\tt a}, or returns
null and sets the error code to indicate an error.
**/

/*
\paragraph{Path arrays.}
{\em Path arrays} encode linear regularity of connections.  A path
array is a set of $n-1$ line segments, where the $i$th line segment
joins points $i$ and $i+1$.  All path arrays have type integer,
category real, rank 1, and shape 2.
*/

PathArray DXNewPathArray(int count);
/**
\index{DXNewPathArray}
Specifies that {\tt a} represents a ``connections'' array consisting
of {\tt count-1} line segments each connecting adjacent points.  The
type of {\tt a} is implicitly set to integer, the rank to 1, and the
shape to 2.  Returns the path array, or returns null and sets the
error code to indicate an error.
**/

PathArray DXGetPathArrayInfo(PathArray a, int *count);
/**
\index{DXGetPathArrayInfo}
If {\tt count} is not null, this routine returns in {\tt *count} the
number of points referred to by the path array {\tt a}; this is one
more than the number of line segments in {\tt a}.  Returns {\tt a}, or
returns null and sets the error code to indicate an error.
**/

PathArray DXSetPathOffset(PathArray a, int offset);
PathArray DXGetPathOffset(PathArray a, int *offset);
/**
\index{DXSetPathOffset}\index{DXGetPathOffset}
In the case where the path array is used to define a regular grid of
connections that is a part of a partitioned field, it is useful to
know the offset of the partition within the original field.  These
routines set and retrieve the offset value for the direction of the
grid represented by this path.  Returns {\tt a}, or returns null and
sets the error code to indicate an error.
**/

/*
\paragraph{Product arrays.}
{\em Product arrays} encode multi-dimensional positional regularity.
A product array is the set of points obtained by summing one
point from each of the terms in all possible combinations,
representing multi-dimensional regular positions.  Each term may be
either regular or not, resulting in either completely or partially
regular multi-dimensional positions.
*/

ProductArray DXNewProductArrayV(int n, Array *terms);
ProductArray DXNewProductArray(int n, ...);
/**
\index{DXNewProductArrayV}\index{DXNewProductArray}
Creates an array that is the product of a set of regular or irregular
position arrays.  All of the array types must be floating point and of
the same rank and shape.  The terms of the product are given by the
array {\tt terms} (for {\tt DXNewProductArrayV()}) or by the last {\tt
n} arguments (for {\tt DXNewProductArray()}).  Returns the product
array, or returns null and sets the error code to indicate an error.
**/

ProductArray DXGetProductArrayInfo(ProductArray a, int *n, Array *terms);
/**
\index{DXGetProductArrayInfo}
If {\tt n} is not null, this routine returns in {\tt *n} the number of
terms in the product {\tt a}.  If {\tt terms} is not null, it returns
in {\tt *terms} the terms of the product. Returns {\tt a}, or returns
null and sets the error code to indicate an error.
**/


/*
\paragraph{Mesh arrays.}
{\em Mesh arrays} encode multi-dimensional regularity of connections.
A mesh array is a product of a set of connections arrays.  The product
is a set of interpolation elements where the product has one
interpolation element for each pair of interpolation elements in the
two multiplcands, and the number of sample points in each
interpolation element is the product of the number of sample points in
each of the multiplicands' interpolation elements.  This represents
multi-dimensional regular connections.  Each term may be either
regular or not, resulting in either completely regular (e.g. cubes) or
partially regular (e.g. prisms) multi-dimensional connections.
*/

MeshArray DXNewMeshArrayV(int n, Array *terms);
MeshArray DXNewMeshArray(int n, ...);
/**
\index{DXNewMeshArrayV}\index{DXNewMeshArray}
Creates an array that is the product of a set of regular or irregular
position arrays.  All of the array types must be floating point and of
the same rank and shape.  The terms of the product are given by the
array {\tt terms} (for {\tt DXNewMeshArrayV()}) or by the last {\tt
n} arguments (for {\tt DXNewMeshArray()}).  This returns the mesh array,
or returns null and sets the error code to indicate an error.
**/

MeshArray DXGetMeshArrayInfo(MeshArray a, int *n, Array *terms);
/**
\index{DXGetMeshArrayInfo}
If {\tt n} is not null, this routine returns in {\tt *n} the number of
terms in the product {\tt a}.  If {\tt terms} is not null, it returns
in {\tt *terms} the terms of the product.  Returns {\tt a}, or returns
null and sets the error code to indicate an error.
**/

MeshArray DXSetMeshOffsets(MeshArray a, int *offsets);
MeshArray DXGetMeshOffsets(MeshArray a, int *offsets);
/**
\index{DXSetMeshOffsets}\index{DXGetMeshOffsets}
In the case where the mesh array is used to define a regular grid of
connections that is a part of a partitioned field, it is useful to
know the offset of the partition within the original field.  These
routines set and retrieve the offset values along each dimension of
the mesh.  For {\tt DXSetMeshOffsets}, the {\tt offsets} parameter is a
pointer to an array of integers, one for each dimension of the mesh,
specifying the offset along that dimension of this partition within
the original field.  {\tt DXGetMeshOffsets} retrieves this information.
Returns {\tt a}, or returns null and sets the error code to indicate
an error.
**/

ConstantArray DXNewConstantArray(int n, Pointer d, Type t,
					Category c, int r, ...);
ConstantArray DXNewConstantArrayV(int n, Pointer d, Type t,
					Category c, int r, int *s);
Array         DXQueryConstantArray(Array a, int *num,  Pointer d);
Pointer	      DXGetConstantArrayData(Array array);

Error 	    DXRegisterSharedSegment(int id,
			    void (*sor)(int, Pointer, Pointer), Pointer d);

SharedArray DXNewSharedArray(int id, Pointer d, int knt, Type t, Category c, int r, ...);
SharedArray DXNewSharedArrayV(int id, Pointer d, int knt, Type t, Category c, int r, int *s);
SharedArray DXNewSharedArrayFromOffset(int id, long offset, int knt, Type t, Category c, int r, ...);
SharedArray DXNewSharedArrayFromOffsetV(int id, long offset, int knt, Type t, Category c, int r, int *s);

/* make string list arrays */
Array DXMakeStringList(int n, char *s, ...);
Array DXMakeStringListV(int n, char **s);

/** given a pointer to a list of n char pointers, make
an array with TYPE_STRING, rank 1, shape length_of_longest_string. **/

Array DXMakeInteger(int n);
Array DXMakeFloat(float f);

#endif /* _DXI_ARRAY_H_ */

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif
