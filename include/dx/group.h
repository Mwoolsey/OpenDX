/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/


#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#define class __class__
#endif

#ifndef _DXI_GROUP_H_
#define _DXI_GROUP_H_

/* TeX starts here. Do not remove this comment. */

/*
\section{Group class}

\paragraph{Members.}
This section describes routines used to manipulate the members of a group.
*/

Group DXNewGroup( void );
/**
\index{DXNewGroup}
Creates a new generic group object.  Returns the group, or returns
null and sets the error code to indicate an error.
**/

Group DXSetMember(Group g, char *name, Object value);
/**
\index{DXSetMember}
Sets the value of a member of a group to {\tt val}, which should be a
field or a group object.  The {\tt name} may be null, in which case a
new member is added that may be accessed only via {\tt
DXGetEnumeratedMember()}.  If {\tt name} is the same as the name of an
existing member, then the new member will have the same index in the
field as the old member.  Returns {\tt g}, or returns null and sets
the error code to indicate an error.
**/

Object DXGetMember(Group g, char *name);
/**
\index{DXGetMember}
Gets the value of the named member of a group.  Returns the member, or
returns null but does not set the error code if the member does not
exist.
**/

Object DXGetEnumeratedMember(Group g, int n, char **name);
/**
\index{DXGetEnumeratedMember}
The members of a group may be enumerated by calling this routine with 
successive values of {\tt n} starting with 0 until null is returned. Note 
that the numbering changes as members are added and deleted. This routine 
returns the name of the {\tt n}th member in {\tt *name} if {\tt name} is not 
null.  Returns the {\tt n}th member, or returns null but does not set the
error code if {\tt n} is out of range.
**/

Group DXSetEnumeratedMember(Group g, int n, Object value);
/**
\index{DXSetEnumeratedMember}
Sets the value of the {\tt n}th member of {\tt g} to {\tt value}; {\tt
n} must refer to an existing member of the group or to the first
non-existent member.  That is, the indices of group members must
always be contiguous starting at 0.  Returns {\tt g}, or returns null
and sets the error code to indicate an error.
**/

Group DXGetMemberCount(Group g, int *n);
/**
\index{DXGetMemberCount}
Returns the number of members in a group in {\tt n}.
**/

Group DXSetGroupType(Group g, Type t, Category c, int rank, ...);
Group DXSetGroupTypeV(Group g, Type t, Category c, int rank, int *shape);
/**
\index{DXSetGroupType}\index{DXSetGroupTypeV}
Associates a type with {\tt g}.  (See Section \ref{arraysec} for a
definition of {\tt Type} and {\tt Category}.)  When the group type is
set, all current members are checked for type, and all members added
subsequently are checked for type.  The type of a group may be
retrieved by {\tt DXGetType()}.  Returns g, or returns null and sets the
error code to indicate an error (for instance, if one of the parts has
a mismatching type).
**/

Group DXUnsetGroupType(Group g);
/**
\index{DXUnsetGroupType}
Removes any association of a type with {\tt g}.  This routine might
be needed if the data component of a group was replaced or removed.
**/

Class DXGetGroupClass(Group g);
/**
\index{DXGetGroupClass}
Returns the subclass of a group object. This will be {\tt CLASS\_GROUP} if
the object is a generic group object, or either {\tt CLASS\_SERIES},
{\tt CLASS\_COMPOSITEFIELD} or {\tt CLASS\_MIXEDFIELD} if the object class
is a subclass of {\tt Group}.
**/

/*
\paragraph{Series groups.}
Series groups are a subclass of {\tt Group} that represent, for example, time
series data.  Every member of the group must have the same dimensionality,
say $n$.  In addition to the $n$ dimensions of each member of the series,
there is a ``series dimension'', for example, time.  Every
member of the series has some position along the series dimension.
*/

Series DXNewSeries( void );
/**
\index{DXNewSeries}
Creates a new series object.  A series is a collection of compatible
fields, all having a ``positions'' component of the same
dimensionality and ``data'' components of the same type.  The
dimensionality and type are set by the first field added to the group;
all subsequent fields must have the same dimensionality and type.  A
series group is intended to be interpreted sequentially.  Returns the
series, or returns null and sets the error code to indicate an error.
**/

Series DXSetSeriesMember(Series s, int n, double position, Object o);
/**
\index{DXSetSeriesMember}
Puts {\tt o} into series {\tt s} as the {\tt n}th member, and records
its position along the series axis.  (Note: {\tt DXSetMember()} and {\tt
DXSetEnumeratedMember()} may also be used, in which case the position is
assumed to be the same as the sequence number of the member.)  The
members of a series must be added with sequential indices {\tt n}
starting with 0.  Returns {\tt s}, or returns null and sets the error
code to indicate an error.
**/

Object DXGetSeriesMember(Series s, int n, float *position);
/**
\index{DXGetSeriesMember}
Returns the {\tt n}th member of series {\tt s}.  If {\tt position} is
not null, returns in {\tt *position} the position of the member along
the series axis.  (Note: {\tt DXGetMember()} and {\tt
DXGetEnumeratedMember()} may also be used for this purpose, except that
they do not return the {\tt position} value.)  Returns the {\tt n}th
member, or returns null but does not set the error code if {\tt n} is
out of range.
**/

/*
\paragraph{Composite field.}
A composite field is a group of fields that together is to be regarded
as one field.  This is useful for example for certain kinds of
simulation data which are represented by disjoint grids.
*/

CompositeField DXNewCompositeField( void );
/**
\index{DXNewCompositeField.}
Creates a new composite field object.  A composite field is a
collection of compatible fields, all having a ``positions'' component
of the same dimensionality and ``data'' components of the same type.
The dimensionality and type are set by the first field added to the
group; all subsequent fields must have the same dimensionality and
type.  A composite field is intended to be interpreted as a collection
of fields that together define a single field.  Returns the group, or
returns null and sets the error code to indicate an error.
**/

/*
\paragraph{Composite field.}
A composite field is a group of fields that together is to be regarded
as one field.  This is useful for example for certain kinds of
simulation data which are represented by disjoint grids.
*/

MultiGrid DXNewMultiGrid( void );
/**
\index{DXNewMultiGrid.}
Creates a new multigrid object.  A multigrid is a collection of
compatible fields, all having a ``positions'' component
of the same dimensionality and ``data'' components of the same type.
The dimensionality and type are set by the first field added to the
group; all subsequent fields must have the same dimensionality and
type.  Like composite fields, multigrid is intended to be interpreted
as a collection of fields that together define a single field.  Unlike
composite fields, however, the data represented by a multigrid is 
not continuous at the boundary.  Returns the group, or
returns null and sets the error code to indicate an error.
**/

/*
\paragraph{Parts.}
This section describes routines to manipulate the parts of a group.
The parts of a group {\tt g} are the fields at the leaves of the tree
of groups under {\tt g}.  In general, a group will be at the root of a
tree that contains groups in the interior nodes and fields at the leaf
nodes.  The fields at the leaves are of course members of their parent
groups, but we also say they are
{\em parts} of the root group:
\begin{center}
\makebox[0pt]{\psfig{figure=gloss.px}}
\end{center}
*/

Field DXGetPart(Object o, int n);
/**
\index{DXGetPart}
The parts of a group may be enumerated by calling {\tt DXGetPart()} with
successive values of {\tt n} starting with 0 until null is returned.
This call is equivalent to {\tt DXGetPartClass()} (q.v.) with the class
parameter set to {\tt CLASS\_FIELD}.  Returns the {\tt n}th part, or
returns null to indicate an error.
**/

Object DXGetPartClass(Object o, int n, Class class);
/**
\index{DXGetPartClass}
The parts of a group may be enumerated by calling {\tt DXGetPart()} with
successive values of {\tt n} starting with 0 until null is returned.
This call returns the successive parts that are of the specified class.
Returns the {\tt n}th part, or returns null to indicate an error.
**/

Object DXSetPart(Object o, int n, Field field);
/**
\index{DXSetPart}
Sets the {\tt n}th part of {\tt g} to {\tt field}.  The field must
already have a defined structure that has at least {\tt n} parts.
Returns {\tt g}, or returns null to indicate an error.
**/

Object DXProcessParts(Object object, Field (*process)(Field, char*, int),
		    Pointer args, int size, int copyit, int preserve);
/**
\index{DXProcessParts}
Applies the {\tt process} function to every constituent field (part)
of the given {\tt object}.  If the input {\tt object} is a field, this
routine returns the result of the {\tt process} function on that
field.

If the input {\tt object} is a group and {\tt copyit} is 1, it makes a
copy of the group and recursively of all subgroups. In this case, the
order of the fields in the groups is preserved if {\tt preserve} is 1.
If this is not required, set {\tt preserve} to 0 and a more efficient
traversal algorithm will be used.

If the input {\tt object} is a group and {\tt copyit} is 0, it
operates directly on the groups of the input {\tt object}.

In either case, for every field {\tt f} that is a member of a group,
it makes a call of the form {\tt process(f, args, size)} and places
the result of that call in the output in place of {\tt f}.  The {\tt
process} function is intended to return a field which is the processed
version of input field {\tt f}.  Typically, the result will be a copy
of the input field {\tt f} with some modifications.

The {\tt size} parameter specifies the size of the block pointed to by
{\tt args}.  If {\tt size} is non-zero, it makes a copy of the
argument block and places it in global memory before passing it to
{\tt process}.  It is required that the argument be in global memory
because {\tt DXProcessParts} runs in parallel; however, if the pointer
passed is, for example, just a pointer to an object which is already
in global memory, then {\tt size} can be given as 0.

Returns the {\tt object}, a copy of it, or a processed version of it
depending on the parameters, or returns null and sets the error code
to indicate an error.
**/

/*
\paragraph{Note.} The {\tt DXGetPart()}, {\tt DXGetPartClass()}, and {\tt
DXSetPart()} routines are not implemented with by an efficient algorithm
in the current version of the Data Explorer, so they are useful
primarily for prototyping or in cases where their convenience
outweighs efficiency concerns.  The {\tt DXProcessParts()} routine can
often be used for the same purposes with better efficiency.
*/

#endif /* _DXI_GROUP_H_ */

#if defined(__cplusplus) || defined(c_plusplus)
#undef class
}
#endif
