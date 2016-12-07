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

#ifndef _DXI_PARTITION_H_
#define _DXI_PARTITION_H_

/* TeX starts here. Do not remove this comment. */

/*
\section{Partitioning}
Partitioning is the process of dividing a field into spatially local pieces.
This is useful both for parallelism and disk access management.

Partitioning divides an input field into a number of spatially local
self-contained fields.  In particular, every interpolation element of
the input field is assigned to one and only one partition.  Thus, the
resulting partitions cover precisely the same region of space as the
input field, without overlap.  However, because the input elements are
left intact, the bounding boxes of the resulting partitions may
overlap.

Partitioning is done as follows.  First, space is divided into
non-overlapping regions (such as boxes) that will form the basis of
the partitions.  Then every point is assigned to one of the partitions
on the basis of which region it is in, and given a number within that
partition.  Next, every element is examined in turn and assigned to
one of the partitions on the basis of which region one of its points
(selected arbitrarily) is in.  Putting an element in a particular
partition may require putting some addition points into that
partition, if the element straddles the border between regions.
Finally, the bounding boxes of the partitions can be computed.  These
in general will not be the same as the regions used to assign points
to partitions, and may indeeed overlap.  In the case of irregular
data, the regions used as the basis of the partitions are computed by
recursive binary subdivision of the field, taking the density of
points into account.  In any case, the result is a {\tt Group} of {\tt
Fields} each representing a partition.
*/

Group DXPartition(Field f, int n, int size);
/**
\index{DXPartition}
Divides {\tt f} into at most {\tt n} spatially local pieces with at least
{\tt size} points in each piece.  Returns {\tt f} if it already meets these
criteria, or returns a new group that contains {\tt f} partitioned, or
returns null and sets the error code to indicate an error.
**/

#endif /* _DXI_PARTITION_H_ */

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif
