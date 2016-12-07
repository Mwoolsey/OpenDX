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

#ifndef _DXI_GROW_H_
#define _DXI_GROW_H_
 
/* TeX starts here. Do not remove this comment. */

/*
\section{DXGrow and DXShrink}

Some modules (for example, filters) require information from the
neighborhood of each point.  Since the partitioning process divides the
data into spatially disjoint subsets for independent processing, these
neighborhoods can be divided among different partitions: for example,
a filter kernel may overlap the boundary between two partitions.  In such
cases, processing any one partition requires that information resident
in neighboring partitions be available.
 
In order to simplify this information sharing, routines have been
included to support temporarily overlapping partitions.  {\tt DXGrow()}
modifies its input field and adds to each partition boundary
information from the partition's neighbors.  Because {\tt DXGrow()}
modifies its input, it is up to the caller to use {\tt DXCopy()} to copy
the input if it must not be modified.  After this boundary information
has been accrued, the processing of the partition may be handled
independently of other partitions since all information required to
produce correct results over the original partition is available
within the partition.  For example, in the case of filtering, boundary
information is added so that wherever a filter kernel is placed within
the original partition, the kernel does not extend outside the grown
partition, producing correct results within the original partition.
After processing the field produced by {\tt DXGrow()}, {\tt DXShrink()}
must be called to shrink any components that have not been shrunk by
the caller, and to remove extra references to the original components
that were put in the field by {\tt DXGrow()}.

The depth of an overlap region is specified when {\tt DXGrow()} is called
by specifying the number of {\it rings} to be accrued.  An element is
said to be in the $k$th ring if it has at least one vertex in the $k$th
ring.  A vertex is in the 0th ring if it exists both in partition and
the neighbor, and is in the $k$th ring if it is not in a lower ring and
an element in ring $k-1$ is incident upon it.  Most frequently, such
modules produce results for each vertex based on the
elements incident on the vertex; this is achieved by requesting that
{\tt grow} includes 1 ring: those elements from neighboring partitions
that are incident on vertices that exist in both partitions.

The treatment of the exterior boundary of regular grid data is
specified by a parameter to {\tt DXGrow()}.  You may specify that the
field not be expanded beyond its boundary, that is that the exterior
partitions not be expanded except on the sides that border other
partitions.  Alternatively you may specify that the field be expanded
beyond its original boundaries, with the new data being filled in one
of three ways: with a constant value; with the replicated value from
the nearest edge point in the original field; or with nothing, only
reserving space for the new data but leaving its contents undefined.

While it is necessary that the footprint of a filter kernel, placed
anywhere within the original partition, not extend past the grown
partition boundary, it is probably not necessary to apply the filter
in the boundary regions accrued from neighbors; these regions
are properly handled during the processing of the neighboring
partition.  We therefore provide routines that allow you to query
the original number of positions and connections, in the case of
irregular grids; or the offset relative to the grown partition and size
of the original partition.

Frequently, modules do not require all components of a field that are
dependent on the positions to be grown.  In order to
avoid both the time and space penalties of accruing information that
will not be required during processing, {\tt DXGrow()} requires the calling
application to specify which components, in addition to positions and
connections, will be required.

Modules using {\tt DXGrow()} have the option of producing results
corresponding with the positions of the larger grown field; or, more
efficiently, producing results corresponding only to positions of the
original smaller field.  Even though the latter method is less
efficient, involving more data movement and perhaps more calculation,
it is sometimes more convenient.  Therefore, the {\tt DXShrink()}
function is provided to shrink all components that depend on or
reference positions or connections back to the original size.  If the
user has already shrunk the positions, {\tt DXShrink()} will leave them
unmodified.  In any case, the {\tt DXShrink()} function must be called
after operating on a grown field in order to remove references to the
original components that were placed in the field by {\tt DXGrow()} for
later use by {\tt DXShrink()}.

Both {\tt DXGrow()} and {\tt DXShrink()} operate in parallel on composite fields.
For this reason, {\tt DXGrow()} must be called prior to any subtasking invoked
explicitly by the calling application, and similarly, {\tt DXShrink()} must be
called after any such subtasking has been completed.
*/
 
#define GROW_NONE      NULL
#define GROW_REPLICATE ((Pointer)1)
#define GROW_NOFILL    ((Pointer)2)

Object DXGrow(Object object, int n, Pointer fill, ...);
Object DXGrowV(Object object, int n, Pointer fill, char **components);
/**
\index{DXGrow}\index{DXGrowV}
If {\tt object} is a composite field, add information to each
partition of {\tt object} from its neighboring partitions.  If {\tt
object} is a field, return the unmodified field.  If {\tt object} is
any other type, return an error.  The depth of overlap is defined by
{\tt n}, as described above.  The treatment of the boundary of the
field is specified by the {\tt fill} parameter: specify {\tt
GROW\_NONE} for no expansion at the exterior boundary of the entire
field; {\tt GROW\_REPLICATE} to expand at the boundary, replicating
the nearest edge values; {\tt GROW\_NOFILL} to expand the boundary,
leaving space for the extra data but leaving their value undefined;
and specify a pointer to a data item of the correct type to expand at
the boundary and fill it with the specified fill value.  For {\tt
DXGrowV}, the {\tt components} array contains a null-terminated list of
components to be grown; all others remain unaffected.  For {\tt DXGrow()},
the final arguments after {\tt n} consist of a null-terminated list of
the components to be grown.  Returns the input object with
the overlapping data accrued, or returns null and sets the error
code to indicate an error.
**/
 
Field DXQueryOriginalSizes(Field f, int *positions, int *connections);
Field DXQueryOriginalMeshExtents(Field f, int *offsets, int *sizes);
/**
\index{DXQueryOriginalSizes}\index{QueryOriginalGridExtents}
These routines provide the module writer with information about the
size of the original field used as the input to {\tt DXGrow()}.  The
parameter {\tt f} names a field that was produced by {\tt DXGrow()}.  In
the case of {\tt DXQueryOriginalSizes}, if {\tt positions} is not null,
the number of positions in the original field is returned in {\tt
*positions}; if {\tt connections} is not null, the number of
interpolation elements in the original field is returned.  This is
particularly useful in the case of irregular data.

In the case of data defined on a regular mesh of connections, {\tt
DXQueryOriginalMeshExtents} can be used to obtain the offsets of the
original field relative to the grown field, and the sizes of the
original field.  If {\tt offsets} is not null, the offset in each
dimension of the original field is returned in the array pointed to by
{\tt offsets}; if {\tt sizes} is not null, the size in each dimension
of the original field is returned in the array pointed to by {\tt
offsets}.  Returns {\tt f}, or returns null and sets the error code to
indicate an error.
**/

Object DXShrink(Object object);
/**
\index{DXShrink}
DXRemove information added onto each partition of {\tt object} by {\tt
DXGrow}.  Returns the object with accrued data removed, or returns null
and sets the error code to indicate and error.
**/

Object DXInvalidateDupBoundary(Object o);
Object DXRestoreDupBoundary(Object o);
/** return the object with any duplicate boundary points marked invalid.
if there was a previous invalid positions component, it is saved by
the invalidate call and restored by the restore call.
this call is useful when routines need to do pointwise operations on
partitioned fields and don't want to process the same point multiple
times (like when computing statistics).
**/

#endif /* _DXI_GROW_H_ */

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif
